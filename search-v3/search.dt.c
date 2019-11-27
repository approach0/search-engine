#include <float.h> /* for FLT_MAX */
#include <unistd.h> /* for sleep() */

#include "mhook/mhook.h"
#include "common/common.h"
#include "wstring/wstring.h"
#include "indices-v3/indices.h"

#include "config.h"
#include "term-qry.h"
#include "math-search.h"
#include "search.h"
#include "proximity.h"

/* use MaxScore merger */
typedef struct pd_merger *merger_set_iter_t;
#define merger_set_iterator  pd_merger_iterator
#define merger_set_iter_next pd_merger_iter_next
#define merger_set_iter_free pd_merger_iter_free

/* shorthand for level-2 math iterator */
#define math_iter_cur  math_l2_invlist_iter_cur;
#define math_iter_next math_l2_invlist_iter_next;
#define math_iter_skip math_l2_invlist_iter_skip;
#define math_iter_read math_l2_invlist_iter_read;

/* shorthand for max upperbound calculation */
#define MAX_AFTER(_iter, _i) \
	(_i + 1 < (_iter)->size ? (_iter)->acc_max[_i + 1] : -FLT_MAX)

static int
prepare_term_keywords(struct indices *indices, struct query *qry,
	 struct merge_set *ms, struct BM25_scorer *bm25,
	 struct term_qry *term_qry, struct term_invlist_entry_reader *term_reader)
{
	/* initialize term query structures */
	int n_term = 0;
	for (int i = 0; i < qry->len; i++) {
		struct query_keyword *kw = query_get_kw(qry, i);
		char *kw_str = wstr2mbstr(kw->wstr); /* utf-8 */

		if (kw->type == QUERY_KW_TERM) {
			term_qry_prepare(indices->ti, kw_str, term_qry + n_term);
			n_term ++;
		}
	}

	/* merge term query duplicate keywords */
	n_term = term_qry_array_merge(term_qry, n_term);

	/* prepare Okapi-BM25 scoring structure */
	*bm25 = prepare_bm25(indices->ti, term_qry, n_term);
#ifdef PRINT_SEARCH_QUERIES
	// printf("[BM25 parameters]\n");
	// BM25_params_print(bm25);
	printf("\n[inverted lists]\n");
#endif

	/* only generate iterators for unique keywords */
	for (int i = 0; i < n_term; i++) {
		const int n = ms->n;
		uint32_t term_id = term_qry[i].term_id;
		struct term_invlist_entry_reader *reader = term_reader + i;

		/* lookup index entry for this term */
		*reader = term_index_lookup(indices->ti, term_id);

#ifdef PRINT_SEARCH_QUERIES
		printf("[%d] ", n);
#endif
		/* push into merge set */
		if (reader->inmemo_reader) {
#ifdef PRINT_SEARCH_QUERIES
			printf("(in memo) ");
#endif
			ms->iter  [n] = reader->inmemo_reader;
			ms->upp   [n] = term_qry[i].upp;
			ms->pd_upp[n] = term_qry[i].upp * MEDAL_WEIGHT;
			ms->sortby[n] = ms->upp[n];
			ms->cur   [n] = (merger_callbk_cur) invlist_iter_curkey;
			ms->next  [n] = (merger_callbk_next)invlist_iter_next;
			ms->skip  [n] = (merger_callbk_skip)invlist_iter_jump;
			ms->read  [n] = (merger_callbk_read)invlist_iter_read;
		} else if (reader->ondisk_reader) {
#ifdef PRINT_SEARCH_QUERIES
			printf("(on disk) ");
#endif
			ms->iter  [n] = reader->ondisk_reader;
			ms->upp   [n] = term_qry[i].upp;
			ms->pd_upp[n] = term_qry[i].upp * MEDAL_WEIGHT;
			ms->sortby[n] = ms->upp[n];
			ms->cur   [n] = (merger_callbk_cur) term_posting_cur;
			ms->next  [n] = (merger_callbk_next)term_posting_next;
			ms->skip  [n] = (merger_callbk_skip)term_posting_jump;
			ms->read  [n] = (merger_callbk_read)term_posting_read;
		} else {
#ifdef PRINT_SEARCH_QUERIES
			printf("( empty ) ");
#endif
			ms->iter  [n] = NULL;
			ms->upp   [n] = 0;
			ms->pd_upp[n] = 0;
			ms->sortby[n] = -FLT_MAX;
			ms->cur   [n] = empty_invlist_cur;
			ms->next  [n] = empty_invlist_next;
			ms->skip  [n] = empty_invlist_skip;
			ms->read  [n] = empty_invlist_read;
		}

#ifdef PRINT_SEARCH_QUERIES
		printf("%6.2f ", ms->upp[n]);
		term_qry_print(term_qry + i);
#endif
		ms->n += 1;
	}

	return n_term;
}

static int
prepare_math_keywords(struct indices *indices, struct query *qry,
	 struct merge_set *ms, struct math_l2_invlist **math_invlist,
	math_l2_invlist_iter_t *math_iter, float *math_th, float *dynm_th)
{
	int n_math = 0;
	for (int i = 0; i < qry->len; i++) {
		const int n = ms->n;
		struct query_keyword *kw = query_get_kw(qry, i);

		/* only consider math keyword here */
		if (kw->type != QUERY_KW_TEX)
			continue;

		/* math level-2 inverted list and iterator */
		char *kw_str = wstr2mbstr(kw->wstr); /* utf-8 */
		struct math_l2_invlist *minv = math_l2_invlist(indices->mi, kw_str,
			math_th + n_math, dynm_th + n_math);
		math_l2_invlist_iter_t miter = NULL;

		/* generate math iterator only if math invlist is valid */
		if (minv) {
			miter = math_l2_invlist_iterator(minv);
			math_invlist[n_math] = minv;
			math_iter[n_math] = miter;
		}

#ifdef PRINT_SEARCH_QUERIES
		printf("[%d] ", n);
#endif
		/* push into merge set */
		if (minv == NULL || miter == NULL) {
			ms->iter  [n] = NULL;
			ms->upp   [n] = 0;
			ms->pd_upp[n] = 0;
			ms->sortby[n] = -FLT_MAX;
			ms->cur   [n] = empty_invlist_cur;
			ms->next  [n] = empty_invlist_next;
			ms->skip  [n] = empty_invlist_skip;
			ms->read  [n] = empty_invlist_read;

#ifdef PRINT_SEARCH_QUERIES
			printf("( empty ) `%s' (malformed TeX, upp=0)\n", kw_str);
#endif
		} else {
			const float math_upp = math_l2_invlist_iter_upp(miter);
			ms->iter  [n] = miter;
			ms->upp   [n] = math_upp;
			ms->pd_upp[n] = math_upp * MEDAL_WEIGHT;
			ms->sortby[n] = ms->upp[n]; // -(100.f / (1.f + ms->upp[n]));
			ms->cur   [n] = (merger_callbk_cur) math_iter_cur;
			ms->next  [n] = (merger_callbk_next)math_iter_next;
			ms->skip  [n] = (merger_callbk_skip)math_iter_skip;
			ms->read  [n] = (merger_callbk_read)math_iter_read;

#ifdef PRINT_SEARCH_QUERIES
			printf("(level 2) %6.2f `%s' (TeX, upp=%.2f)\n",
				ms->upp[n], kw_str, math_upp);
			math_qry_print(&minv->mq, 0);
#endif
		}

		n_math += 1;
		ms->n += 1;
	}

	return n_math;
}

static struct rank_hit
*new_hit(doc_id_t docID, float score, prox_input_t *prox, int n)
{
	struct rank_hit *hit;
	hit = malloc(sizeof *hit);
	hit->docID = docID;
	hit->score = score;
	hit->occur = malloc(sizeof(uint32_t) * MAX_TOTAL_OCCURS);
	hit->n_occurs = prox_sort_occurs(hit->occur, prox, n);
	return hit;
}

static float prox_upp_relax(void *arg, float upp)
{
	P_CAST(proximity_upp, const float, arg);
	return upp + (*proximity_upp);
}

static inline float internal_threshold(merge_set_t *ms, int iid, int optim,
	float max_before, float max_after, float acc_upp, float threshold)
{
	/* pessimistic */
	float others_max = MAX(max_before, max_after);
	float pess = threshold - (others_max + acc_upp) + ms->upp[iid];

	if (optim || others_max == -FLT_MAX) {
		/* optimistic */
		float opti = threshold - acc_upp + ms->upp[iid];

		opti = opti / (MEDAL_WEIGHT + 1.f);

		return MIN(opti, pess);
	}

	return pess;
}

#ifdef DEBUG_L2_SEARCH
static int inspect(uint64_t docID)
{
	return (docID == 142544);
}
#endif

ranked_results_t
indices_run_query(struct indices* indices, struct query* qry)
{
	/* top-K results' heap */
	ranked_results_t rk_res;
	priority_Q_init(&rk_res, DEFAULT_N_TOP_RESULTS);

	/* merge set */
	struct merge_set merge_set = {0};

	/*
	 * Prepare term query keywords
	 */
	struct term_qry term_qry[qry->n_term];
	struct term_invlist_entry_reader term_reader[qry->n_term];

	memset(term_qry, 0, sizeof term_qry);
	memset(term_reader, 0, sizeof term_reader);

	struct BM25_scorer bm25;
	int n_term = prepare_term_keywords(indices, qry, &merge_set,
	                                   &bm25, term_qry, term_reader);

	/*
	 * Prepare math query keywords
	 */
	struct math_l2_invlist *math_invlist[qry->n_math];
	math_l2_invlist_iter_t math_iter[qry->n_math];
	float math_threshold[qry->n_math];
	float dynm_threshold[qry->n_math];

	memset(math_invlist, 0, sizeof math_invlist);
	memset(math_iter, 0, sizeof math_iter);
	memset(math_threshold, 0, sizeof math_threshold);
	memset(dynm_threshold, 0, sizeof dynm_threshold);

	int n_math = prepare_math_keywords(indices, qry, &merge_set,
		math_invlist, math_iter, math_threshold, dynm_threshold);

	/*
	 * Threshold and proximity upperbound
	 */
	float threshold = 0.f;
	const float proximity_upp = prox_upp();
	// printf("proximity upperbound: %.2f\n", proximity_upp);

	/*
	 * Merge variables
	 */
	struct term_posting_item term_item[qry->n_term];
	struct math_l2_iter_item math_item[qry->n_math];
	prox_input_t prox[qry->len];

#ifdef DEBUG_INDICES_RUN_QUERY
	foreach (iter, merger_set, &merge_set) {
		pd_merger_iter_print(iter, NULL);
		sleep(3);
		printf("Begin merging !!!\n");
		break;
	}
#endif

	/*
	 * Perform merging
	 */
	foreach (iter, merger_set, &merge_set) {
		/*
		 * get proximity score (only consider term keywords)
		 */
		int h = 0; /* number of hits */
		for (int i = 0; i < iter->size; i++) {
			int iid = iter->map[i];

			if (iid < n_term) {
				/* non-requirement set follows up (for text keyword) */
				if (i > iter->pivot) {
					if (!pd_merger_iter_follow(iter, iid))
						continue;
				}

				uint64_t cur = merger_map_call(iter, cur, i);
				if (cur == iter->min) {
					/* read hit items and use their occurred positions */
					struct term_posting_item *item = term_item + iid;
					merger_map_call(iter, read, i, item, sizeof *item);
					prox_set_input(prox + (h++), item->pos_arr, item->n_occur);
				}
			}
		}
		/* calculate proximity score */
		float proximity = prox_score(prox, h);

#ifdef DEBUG_L2_SEARCH
		if (inspect(iter->min)) {
			printf(ES_RESET_CONSOLE);
			pd_merger_iter_print(iter, NULL);
			printf("\n");
			usleep(500);
		}
#endif
		/*
		 * calculate TF-IDF / SF-IPF score
		 */
		float acc_score = proximity; /* accumulated score */
		float acc_max = -FLT_MAX; /* accumulated max score */
		for (int i = 0; i < iter->size; i++) {
			float max_score = MAX(acc_max, iter->acc_max[i]);

			/* prune document early if it cannot make into top-K */
			if (max_score + acc_score + iter->acc_upp[i] < threshold) {
				goto next_iter;
			}

			/* non-requirement set follows up (for math keyword) */
			int iid = iter->map[i];
			if (i > iter->pivot) {
				if (!pd_merger_iter_follow(iter, iid))
					continue;
			}

			/* only consider hit iterators */
			uint64_t cur = merger_map_call(iter, cur, i);
			if (cur != iter->min)
				continue;

			/* accumulate precise partial score */
			if (iid < n_term) {
				struct term_qry *tq = term_qry + iid;
				struct term_posting_item *item = term_item + iid;

				/* accumulate TF-IDF score */
				float l = term_index_get_docLen(indices->ti, item->doc_id);
				float s = BM25_partial_score(&bm25, item->tf, tq->idf, l);
				s = s * tq->qf;
				acc_score += s;
				acc_max = MAX(acc_max, s * MEDAL_WEIGHT);

			} else {
				int mid = iid - n_term;

				/* update math level-2 iterator dynamic threshold */
				float acc_upp = acc_score + iter->acc_upp[i];

				dynm_threshold[mid] = internal_threshold(&iter->set, iid,
					iter->set.pd_upp[iid] >= acc_max /* true => optimistic */,
					acc_max, MAX_AFTER(iter, i),
					acc_upp, threshold);

				/* math level-2 iterator read item */
				struct math_l2_iter_item *item = math_item + mid;
				merger_map_call(iter, read, i, item, sizeof *item);

				/* accumulate SF-IPF score */
				acc_score += item->score;
				acc_max = MAX(acc_max, item->score * MEDAL_WEIGHT);

				/* save math term position for later being highlighted */
				prox_set_input(prox + (h++), item->occur, item->n_occurs);
			}
		}

#ifdef DEBUG_INDICES_RUN_QUERY
		/*
		 * print iteration
		 */
		// pd_merger_iter_print(iter, NULL);
		// printf("\n");
#endif

		/*
		 * update result only if needed
		 */
		float doc_score = acc_max + acc_score;

		/* if top-K is not full or this score exceeds threshold */
		if (!priority_Q_full(&rk_res) ||
			doc_score > priority_Q_min_score(&rk_res)) {

			/* store and push this candidate hit */
			struct rank_hit *hit;
			hit = new_hit(iter->min, doc_score, prox, h);
			priority_Q_add_or_replace(&rk_res, hit);

			/* update threshold if the heap is full */
			if (priority_Q_full(&rk_res)) {
				/* update threshold */
				threshold = priority_Q_min_score(&rk_res);
#ifdef DEBUG_INDICES_RUN_QUERY
				printf(ES_RESET_CONSOLE);
				printf("threshold -> %.2f\n", threshold);
#endif

				/* update math static thresholds */
				acc_max = -FLT_MAX; /* accumulated max score */
				for (int i = 0; i < iter->size; i++) {
					int iid = iter->map[i];
					int mid = iid - n_term;
					if (mid < 0) { continue; }

					math_threshold[mid] = internal_threshold(&iter->set, iid,
						1 /* any keyword can become the one with max score */,
						acc_max, MAX_AFTER(iter, i),
						iter->acc_upp[0], threshold);

					acc_max = MAX(acc_max, iter->set.pd_upp[iid]);

#ifdef DEBUG_INDICES_RUN_QUERY
					if (math_iter[mid]) {
						printf("[%d] dynm_threshold -> %.2f",
							iid, dynm_threshold[mid]);
						printf(" math_threshold -> %.2f\n",
							math_threshold[mid]);
						printf("\t");
						math_pruner_print_stats(math_iter[mid]->pruner);
					}
#endif
				}

				/* update pivot */
				pd_merger_lift_up_pivot(iter, threshold,
				                        prox_upp_relax, (void*)&proximity_upp);
#ifdef DEBUG_INDICES_RUN_QUERY
				pd_merger_iter_print(iter, NULL);
				printf("\n");
				fflush(stdout);
#endif
			}
		}

next_iter:
		;
	} /* end of merge */

	/*
	 * Release term query keywords
	 */
	for (int i = 0; i < n_term; i++) {
		/* free term query reader */
		struct term_invlist_entry_reader *reader = term_reader + i;
		if (reader->inmemo_reader)
			invlist_iter_free(reader->inmemo_reader);
		else if (reader->ondisk_reader)
			term_posting_finish(reader->ondisk_reader);

		term_qry_release(term_qry + i);
	}

	/*
	 * Release math query keywords
	 */
	for (int i = 0; i < n_math; i++) {
		struct math_l2_invlist *minv = math_invlist[i];
		math_l2_invlist_iter_t miter = math_iter[i];
		if (miter)
			math_l2_invlist_iter_free(miter);
		if (minv)
			math_l2_invlist_free(minv);
	}

	/* before returning top-K, invoke heap-sort. */
	priority_Q_sort(&rk_res);
	return rk_res;
}
