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
#include "bm25-score.h"

static struct rank_hit
*new_hit(doc_id_t hitID, float score, prox_input_t *prox, int n)
{
	struct rank_hit *hit;
	hit = malloc(sizeof(struct rank_hit));
	hit->docID = hitID;
	hit->score = score;
	hit->occurs = malloc(sizeof(hit_occur_t) * MAX_TOTAL_OCCURS);
	hit->n_occurs = prox_sort_occurs(hit->occurs, prox, n);
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

	/* merger */
	struct postmerger root_pm;
	postmerger_init(&root_pm);

	/* search result heap */
	ranked_results_t rk_res;
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

#ifdef MERGE_TIME_LOG
	// fprintf(mergetime_fh, "checkpoint-a %ld msec.\n", timer_tot_msec(&timer));
#endif
	uint32_t sep = 0; /* term / math postlist separator */
	struct BM25_score_args  bm25; /* Okapi BM25 arguments */
	struct term_qry_struct  tqs[qry->n_term]; /* term TF-IDF information */
	struct math_qry_struct  mqs[qry->n_math]; /* "TeX" tree/path structure */
	struct math_l2_postlist mpo[qry->n_math]; /* math level-2 posting list */

	/* prepare BM25 global arguments */
#ifndef IGNORE_TERM_INDEX
	bm25.avgDocLen = (float)term_index_get_avgDocLen(indices->ti);
	bm25.docN = (float)term_index_get_docN(indices->ti);
#else
	bm25.avgDocLen = 1.f;
	bm25.docN = 1.f;
#endif
	bm25.b  = BM25_DEFAULT_B;
	bm25.k1 = BM25_DEFAULT_K1;
	bm25.frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25.avgDocLen;

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

		/* BM25 per-term global arguments */
		bm25.idf[i] = BM25_idf(&bm25, tqs[i].df);

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
	printf("\n");

	// Initialize merger iterators
	for (int j = 0; j < root_pm.n_po; j++) {
		/* calling math_l2_postlist_init() */
		POSTMERGER_POSTLIST_CALL(&root_pm, init, j);
	}
#ifdef MERGE_TIME_LOG
	// fprintf(mergetime_fh, "checkpoint-init %ld msec.\n", timer_tot_msec(&timer));
#endif

#ifdef ENABLE_PROXIMITY_SCORE
	/* proximity score data structure */
	prox_input_t prox[qry->len];
#endif

#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
	uint64_t cnt = 0;
#endif

#ifdef SKIP_SEARCH
	goto skip_search;
#endif

	/* allocate posting list items storage */
	struct math_l2_postlist_item math_item[qry->n_math];
	struct term_posting_item     term_item[qry->n_term];
	hit_occur_t term_pos[qry->n_term][MAX_TOTAL_OCCURS];

	/* merge top-level posting lists here */
	foreach (iter, postmerger, &root_pm) {
		int h = 0; /* number of hit lists */
		uint64_t cur;
		float max_math_score = 0.f;
		float tf_idf_score = 0.f;

#ifdef PRINT_RECUR_MERGING_ITEMS
		printf("min docID: %u\n", iter->min);
#endif

#ifndef IGNORE_TERM_INDEX
		/* document length */
		uint32_t l = term_index_get_docLen(indices->ti, iter->min);
#else
		uint32_t l = 1;
#endif

		for (int i = 0; i < iter->size; i++) {
			uint32_t pid = iter->map[i];
			cur = postmerger_iter_call(&root_pm, iter, cur, i);

#ifdef PRINT_RECUR_MERGING_ITEMS
			printf("iter[%u] po[%u] = %u\n", i, pid, cur);
#endif
			if (cur != UINT64_MAX && cur == iter->min) {
				if (pid < sep) {
					const int j = i;
					struct term_posting_item *ti = term_item + j;

					POSTMERGER_POSTLIST_CALL(&root_pm, read, pid,
						ti, sizeof(struct term_posting_item));

					/* accumulate term score */
					tf_idf_score += BM25_tf_partial_score(&bm25, j, ti->tf, l);

#ifdef PRINT_RECUR_MERGING_ITEMS
					printf("hit term po[%u]: ", iter->map[i]);
					printf("%s=%u, ", STRVAR_PAIR(ti->doc_id));
					printf("%s=%u, ", STRVAR_PAIR(ti->tf));
					printf("%s=%u: ", STRVAR_PAIR(ti->n_occur));
					for (int k = 0; k < ti->n_occur; k++)
						printf("%u ", ti->pos_arr[k]);
					printf("\n");
#endif
#ifdef ENABLE_PROXIMITY_SCORE
					P_CAST(ti_occurs, hit_occur_t, term_pos + j);
					for (int k = 0; k < ti->n_occur; k++)
						ti_occurs[k].pos = ti->pos_arr[k];
					prox_set_input(prox + h, ti_occurs, ti->n_occur);
#endif

				} else {
					const int j = i - sep;
					struct math_l2_postlist_item *mi = math_item + j;

					POSTMERGER_POSTLIST_CALL(&root_pm, read, pid,
						mi, sizeof(struct math_l2_postlist_item));

					/* find math best score */
					if (mi->part_score > max_math_score)
						max_math_score = mi->part_score;

#ifdef PRINT_RECUR_MERGING_ITEMS
					printf("hit math po[%u]: ", iter->map[i]);
					printf("%s=%u, ",   STRVAR_PAIR(mi->doc_id));
					printf("%s=%.3f, ", STRVAR_PAIR(mi->part_score));
					printf("%s=%u: ",   STRVAR_PAIR(mi->n_occurs));
					for (int k = 0; k < mi->n_occurs; k++)
						printf("%u ", mi->occurs[k].pos);
					printf("\n");
#endif
#ifdef ENABLE_PROXIMITY_SCORE
					prox_set_input(prox + h, mi->occurs, mi->n_occurs);
#endif
				}

				/* count the number of hits in this iteration */
				h ++;
				/* advance posting iterators */
				postmerger_iter_call(&root_pm, iter, next, i);
			}

		}

#ifdef PRINT_RECUR_MERGING_ITEMS
		printf("\n");
#endif

#ifdef ENABLE_PROXIMITY_SCORE
		/* calculate proximity score */
		float prox_score = proximity_score(prox, h);
		prox_reset_inputs(prox, h); /* reset */
#endif

		/* now, calculate the overall score for ranking */
#if 1
		float math_score = (1.f + max_math_score) / 2.f;
		float text_score = 1.f + tf_idf_score;
		float doc_score = prox_score + math_score * text_score;
#else
		float doc_score = max_math_score; /* math-only search */
#endif

		/* if anything hits, consider as top-K candidate */
		if (h) {
			topk_candidate(&rk_res, (doc_id_t)cur, doc_score, prox, h);
			h = 0;
		}

#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
		if (cnt ++ > DEBUG_MERGE_LIMIT_ITERS) {
			printf("abort (DEBUG_MERGE_LIMIT_ITERS).\n");
			break;
		}
#endif
	} /* end of posting lists merge */

#ifdef MERGE_TIME_LOG
	fprintf(mergetime_fh, "mergecost %ld msec.\n", timer_tot_msec(&timer));
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
