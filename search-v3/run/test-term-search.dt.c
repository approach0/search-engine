#include "mhook/mhook.h"
#include "common/common.h"
#include "config.h"
#include "indices-v3/indices.h"
#include "merger/mergers.h"
#include "term-search.h"
#include "proximity.h"
#include "rank.h"
#include "print-search-results.h"

//#define DEBUG_TERM_SEARCH

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator  ms_merger_iterator
#define merger_set_iter_next ms_merger_iter_next
#define merger_set_iter_free ms_merger_iter_free

static void print_term_item(struct term_posting_item *item, int iid)
{
	printf("[read] item[%d] doc#%u: tf=%u, occurred: ", iid,
		item->doc_id, item->tf);
	for (int i = 0; i < item->n_occur; i++) {
		printf("@%u ", item->pos_arr[i]);
	}
	printf("\n");
}

static struct rank_hit
*new_hit(doc_id_t hitID, float score, prox_input_t *prox, int n)
{
	struct rank_hit *hit;
	hit = malloc(sizeof *hit);
	hit->docID = hitID;
	hit->score = score;
	hit->occur = malloc(sizeof(uint32_t) * MAX_TOTAL_OCCURS);
	hit->n_occurs = prox_sort_occurs(hit->occur, prox, n);
	return hit;
}

int main()
{
	struct indices indices;
	char indices_path[] = "../indexerd/tmp";
	//char indices_path[] = "/home/tk/nvme0n1/mnt-test-opt-prune-128-compact.img";

	const char query[][128] = {
		"common", "world", "real", "world", "kunming"
	};
	int qry_len = sizeof(query) / 128;

	struct term_qry term_qry[qry_len];
	struct term_invlist_entry_reader readers[qry_len];
	prox_input_t prox[qry_len];
	struct term_posting_item item[qry_len];

	if(indices_open(&indices, indices_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	/* cache index */
	indices.ti_cache_limit = 16 MB;
	indices.mi_cache_limit = 0;
	indices_cache(&indices);
	indices_print_summary(&indices);
	printf("\n");

	/* prepare term queries */
	for (int i = 0; i < qry_len; i++) {
		term_qry_prepare(indices.ti, query[i], term_qry + i);
	}

	qry_len = term_qry_array_merge(term_qry, qry_len);

	struct BM25_scorer bm25 = prepare_bm25(indices.ti, term_qry, qry_len);

	/* print prepared term queries */
	BM25_params_print(&bm25);
	for (int i = 0; i < qry_len; i++) {
		term_qry_print(term_qry + i);
	}
	printf("\n");

	/* prepare min-heap */
	ranked_results_t rk_res;
	priority_Q_init(&rk_res, DEFAULT_N_TOP_RESULTS);

	/* prepare merger */
	struct merge_set merge_set = {0};
	for (int i = 0; i < qry_len; i++) {
		readers[i] = term_index_lookup(indices.ti, term_qry[i].term_id);
		printf("[%u] `%s' ", i, term_qry[i].kw_str);

		if (readers[i].inmemo_reader) {
			printf("(in memo)");
			merge_set.iter[i] = readers[i].inmemo_reader;
			merge_set.upp [i] = term_qry[i].upp;
			merge_set.cur [i] =  (merger_callbk_cur)invlist_iter_curkey;
			merge_set.next[i] = (merger_callbk_next)invlist_iter_next;
			merge_set.skip[i] = (merger_callbk_skip)invlist_iter_jump;
			merge_set.read[i] = (merger_callbk_read)invlist_iter_read;

		} else if (readers[i].ondisk_reader) {
			printf("(on disk)");
			merge_set.iter[i] = readers[i].ondisk_reader;
			merge_set.upp [i] = term_qry[i].upp;
			merge_set.cur [i] =  (merger_callbk_cur)term_posting_cur;
			merge_set.next[i] = (merger_callbk_next)term_posting_next;
			merge_set.skip[i] = (merger_callbk_skip)term_posting_jump;
			merge_set.read[i] = (merger_callbk_read)term_posting_read;

		} else {
			printf("(empty)");
			merge_set.iter[i] = NULL;
			merge_set.upp [i] = 0;
			merge_set.cur [i] = empty_invlist_cur;
			merge_set.next[i] = empty_invlist_next;
			merge_set.skip[i] = empty_invlist_skip;
			merge_set.read[i] = empty_invlist_read;

		}
		printf("\n");

		merge_set.n += 1;
	}

	float threshold = 0.f;

	/* merge inverted lists */
#ifdef DEBUG_TERM_SEARCH
	int cnt = 0;
#endif
	foreach (iter, merger_set, &merge_set) {
		float tfidf_score = 0.f;
		int h = 0; /* number of hit keywords */

		/*
		 * get proximity score
		 */
		for (int i = 0; i < iter->size; i++) {
			/* advance those in skipping set */
			if (i > iter->pivot) {
#ifdef DEBUG_TERM_SEARCH
				printf("skipping [%d]\n", iter->map[i]);
#endif
				if (!ms_merger_iter_follow(iter, iter->map[i]))
					continue;
			}

			uint64_t cur = merger_map_call(iter, cur, i);
			if (cur == iter->min) {
				/* read hit items and use their occurred positions */
				merger_map_call(iter, read, i, &item[i], sizeof item[i]);
				prox_set_input(prox + (h++), item[i].pos_arr, item[i].n_occur);
			}
		}
		
		float proximity = prox_score(prox, h);
		float delta = threshold - proximity;

		/*
		 * calculate TF-IDF score
		 */
		for (int i = 0; i < iter->size; i++) {
			/* prune document early if it cannot make into top-K */
			if (tfidf_score + iter->acc_upp[i] < delta) {
#ifdef DEBUG_TERM_SEARCH
				printf("doc#%lu pruned. \n\n", iter->min);
#endif
				tfidf_score = 0.f;
				break;
			}

			/* accumulate precise partial score */
			uint64_t cur = merger_map_call(iter, cur, i);
			if (cur == iter->min) {
				int iid = iter->map[i];
#ifdef DEBUG_TERM_SEARCH
				print_term_item(&item[i], iid);
#endif
				/* calculate precise partial score */
				struct term_qry *q = term_qry + iid;
				float s, l = term_index_get_docLen(indices.ti, item[i].doc_id);
				s = BM25_partial_score(&bm25, item[i].tf, q->idf, l);
				tfidf_score += s * q->qf;
#ifdef DEBUG_TERM_SEARCH
				printf("parital[%u] += %.2f x %.0f = %.2f\n", iid,
					s, q->qf, tfidf_score);
#endif
			}
		}

#ifdef DEBUG_TERM_SEARCH
		ms_merger_iter_print(iter, NULL);
#endif
		if (tfidf_score > 0.f) {

#ifdef DEBUG_TERM_SEARCH
			prinfo("[doc#%lu tf-idf=%.2f, prox=%.2f]\n", iter->min,
			       tfidf_score, proximity);
#endif

			float doc_score = tfidf_score + proximity;
			/* push document into top-K and update threshold */
			if (!priority_Q_full(&rk_res) ||
			    doc_score > priority_Q_min_score(&rk_res)) {

				struct rank_hit *hit;
				hit = new_hit(iter->min, doc_score, prox, h);
				priority_Q_add_or_replace(&rk_res, hit);

				/* update threshold if the heap is full */
				if (priority_Q_full(&rk_res)) {
					threshold = priority_Q_min_score(&rk_res);
#ifdef DEBUG_TERM_SEARCH
					printf("update threshold -> %.2f\n", threshold);
#endif
					ms_merger_lift_up_pivot(iter, threshold,
					                        no_upp_relax, NULL);
				}
			}
		}

#ifdef DEBUG_TERM_SEARCH
		printf("\n");

		if (cnt > 100)
			break;
		else
			cnt ++;
#endif
	}

	/* free merger members and term queries */
	for (int i = 0; i < qry_len; i++) {
		if (readers[i].inmemo_reader)
			invlist_iter_free(readers[i].inmemo_reader);
		else if (readers[i].ondisk_reader)
			term_posting_finish(readers[i].ondisk_reader);

		term_qry_release(term_qry + i);
	}

	/* sort and print search results */
	priority_Q_sort(&rk_res);
	print_search_results(&indices, &rk_res, 1);
	priority_Q_free(&rk_res);

close:
	indices_close(&indices);
	mhook_print_unfree();
	return 0;
}
