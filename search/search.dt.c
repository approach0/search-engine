#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common/common.h"
#include "timer/timer.h"
#include "postmerge/postcalls.h"
#include "wstring/wstring.h"

#include "config.h"
#include "math-search.h"
#include "term-search.h"
#include "search.h"
#include "proximity.h"

static uint32_t
sort_occurs(hit_occur_t *dest, prox_input_t* in, int n)
{
#define CUR_POS(_in, _i) \
	_in[_i].pos[_in[_i].cur].pos
	uint32_t dest_end = 0;
	while (dest_end < MAX_HIGHLIGHT_OCCURS) {
		uint32_t i, min_idx, min_cur, min = MAX_N_POSITIONS;
		for (i = 0; i < n; i++)
			if (in[i].cur < in[i].n_pos)
				if (CUR_POS(in, i) < min) {
					min = CUR_POS(in, i);
					min_idx = i;
					min_cur = in[i].cur;
				}
		if (min == MAX_N_POSITIONS)
			/* input exhausted */
			break;
		else
			/* consume input */
			in[min_idx].cur ++;
		if (dest_end == 0 || /* first put */
		    dest[dest_end - 1].pos != min /* unique */)
			dest[dest_end++] = in[min_idx].pos[min_cur];
	}
	return dest_end;
}

static struct rank_hit
*new_hit(doc_id_t hitID, float score, prox_input_t *prox, int n)
{
	struct rank_hit *hit;
	hit = malloc(sizeof(struct rank_hit));
	hit->docID = hitID;
	hit->score = score;
	hit->occurs = malloc(sizeof(hit_occur_t) * MAX_HIGHLIGHT_OCCURS);
	hit->n_occurs = sort_occurs(hit->occurs, prox, n);
	return hit;
}

static void
topk_candidate(ranked_results_t *rk_res,
               doc_id_t docID, float score,
               prox_input_t *prox, uint32_t n)
{
	if (!priority_Q_full(rk_res) ||
	    score > priority_Q_min_score(rk_res)) {
		struct rank_hit *hit = new_hit(docID, score, prox, n);
		priority_Q_add_or_replace(rk_res, hit);
	}
}

ranked_results_t
indices_run_query(struct indices* indices, struct query* qry)
{
#ifdef MERGE_TIME_LOG
	struct timer timer;
	timer_reset(&timer);
	FILE *mergetime_fh = fopen(MERGE_TIME_LOG, "a");
#endif

	struct postmerger root_pm;
	postmerger_init(&root_pm);

	ranked_results_t rk_res;
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

#ifdef MERGE_TIME_LOG
	// fprintf(mergetime_fh, "checkpoint-a %ld msec.\n", timer_tot_msec(&timer));
#endif
	uint32_t sep = 0; /* term / math postlist separator */
	struct term_qry_struct  tqs[qry->n_term]; /* term TF-IDF information */
	struct math_qry_struct  mqs[qry->n_math]; /* "TeX" tree/path structure */
	struct math_l2_postlist mpo[qry->n_math]; /* math level-2 posting list */

#define SKIP_SEARCH ///////////////////////////////
	// Prepare term posting list iterators
	for (int i = 0; i < qry->len; i++) {
		struct query_keyword *kw = query_keyword(qry, i);

		if (kw->type == QUERY_KEYWORD_TERM) {
			char *kw_str = wstr2mbstr(kw->wstr);
			struct term_qry_struct *ts = tqs + (sep ++);

			(void)term_qry_prepare(indices, kw_str, ts);
		}
	}

	// Preprocess term query keywords (merge duplicates)
	sep = term_query_merge(tqs, qry->n_term);

	// Insert term posting list iterators
	for (int i = 0; i < sep; i++) {
		if (tqs[i].term_id == 0)
			root_pm.po[i] = empty_postlist();
		else
			root_pm.po[i] = postmerger_term_postlist(indices, tqs + i);
		root_pm.n_po += 1;
	}


	// Insert math posting list iterators
	for (int k = 0; k < qry->len; k++) {
		struct query_keyword *kw = query_keyword(qry, k);

		if (kw->type == QUERY_KEYWORD_TEX) {
			char *kw_str = wstr2mbstr(kw->wstr);
			const int i = root_pm.n_po;
			const int j = i - sep;
			struct math_qry_struct *ms = mqs + j;

			/* construct level-2 postlist */
			if (0 == math_qry_prepare(indices, kw_str, ms)) {
				mpo[j] = math_l2_postlist(indices, ms, &rk_res /* top-K */);

				root_pm.po[i] = postmerger_math_l2_postlist(mpo + j);
			} else {
				/* math parse error */
				root_pm.po[i] = empty_postlist();
			}

			root_pm.n_po += 1;
		}
	}


	// Print processed query
	//(void)math_postlist_cache_list(indices->ci.math_cache, true);
	for (int i = 0; i < root_pm.n_po; i++) {
		if (i < sep) { /* term query */
			printf("po[%u] `%s' (id=%u, qf=%u, df=%u)\n", i, tqs[i].kw_str,
			       tqs[i].term_id, tqs[i].qf, tqs[i].df);
		} else { /* math query */
			const int j = i - sep;
			printf("po[%u] `%s'\n", i, mqs[j].kw_str);
			math_l2_postlist_print(mpo + j); /* print sub-level posting lists */
		}
	}

	// Initialize merger iterators
	for (int j = 0; j < root_pm.n_po; j++) {
		/* calling math_l2_postlist_init() */
		POSTMERGER_POSTLIST_CALL(&root_pm, init, j);
	}
#ifdef MERGE_TIME_LOG
	// fprintf(mergetime_fh, "checkpoint-init %ld msec.\n", timer_tot_msec(&timer));
#endif

	/* proximity score data structure */
	prox_input_t prox[qry->len];

	// MERGE l2 posting lists here
#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
	uint64_t cnt = 0;
#endif

#ifdef SKIP_SEARCH
	goto skip_search;
#endif

	foreach (iter, postmerger, &root_pm) {
		float math_score = 0.f;
		uint32_t doc_id  = 0;

		for (int i = 0; i < iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&root_pm, iter, cur, i);

			if (cur != UINT64_MAX && cur == iter->min) {
				struct l2_postlist_item item;
				postmerger_iter_call(&root_pm, iter, read, i, &item, 0);

#ifdef PRINT_RECUR_MERGING_ITEMS
				printf("root[%u]: ", iter->map[i]);
				printf("%s=%u, ", STRVAR_PAIR(item.doc_id));
				printf("%s=%u, ", STRVAR_PAIR(item.part_score));
				printf("%s=%u: ", STRVAR_PAIR(item.n_occurs));
				for (int j = 0; j < item.n_occurs; j++)
					printf("%u ", item.occurs[j].pos);
				if (item.n_occurs == MAX_HIGHLIGHT_OCCURS) printf("(maximum)");
				printf("\n\n");
#endif
				doc_id     = item.doc_id;
				math_score = (float)item.part_score;
				prox_set_input(prox + i, item.occurs, item.n_occurs);

				/* advance posting iterators */
				postmerger_iter_call(&root_pm, iter, next, i);
			}
		}

		float prox_score = proximity_score(prox, iter->size);
		(void)prox_score;
		// float doc_score = prox_score + math_score;
		float doc_score = math_score;
		if (doc_score) {
			topk_candidate(&rk_res, doc_id, doc_score, prox, iter->size);
		}

#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
		if (cnt ++ > DEBUG_MERGE_LIMIT_ITERS) {
			printf("abort (DEBUG_MERGE_LIMIT_ITERS).\n");
			break;
		}
#endif
	}

#ifdef MERGE_TIME_LOG
	fprintf(mergetime_fh, "mergecost %ld msec.\n", timer_tot_msec(&timer));
#endif

#ifdef DEBUG_STATS_HOT_HIT
	for (int i = 0; i < 128; i++) {
		if(g_hot_hit[i])
			printf("%d hits freq: %lu\n", i, g_hot_hit[i]);
	}
#endif

#ifdef SKIP_SEARCH
skip_search:
#endif

	// Destroy merger iterators
	for (int j = 0; j < root_pm.n_po; j++) {
		/* calling math_l2_postlist_uninit() */
		POSTMERGER_POSTLIST_CALL(&root_pm, uninit, j);
	}

	// Free math query structure
	for (int j = sep; j < root_pm.n_po; j++) {
		math_qry_free(mqs + (j - sep));
	}

#ifdef MERGE_TIME_LOG
	// fprintf(mergetime_fh, "checkpoint-unit %ld msec.\n", timer_tot_msec(&timer));
#endif

	// Sort min-heap
	priority_Q_sort(&rk_res);
	//////////fflush(stdout);
#ifdef MERGE_TIME_LOG
	// fprintf(mergetime_fh, "checkpoint-Q %ld msec.\n", timer_tot_msec(&timer));
	fclose(mergetime_fh);
#endif
	return rk_res;
}
