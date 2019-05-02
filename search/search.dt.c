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

struct overall_scores {
	float math_score, text_score, doc_score;
};

#ifdef DEBUG_HIT_SCORE_INSPECT
doc_id_t debug_hit_doc;
#endif

#ifdef DEBUG_MATH_MERGE
static struct timer debug_timer;
static int debug_slowdown;
int debug_print;

int debug_search_slowdown()
{
	debug_slowdown = !debug_slowdown;
	return debug_slowdown;
}
#endif

void calc_overall_scores(struct overall_scores *s,
	float math_score, float tf_idf_score, float prox_score)
{
#if 1
	float fm = 0.5f + math_score * 10.f;
	float ft = (1.f + tf_idf_score + prox_score) / 2.f;
	s->math_score = math_score;
	s->text_score = (ft > 0.f) ? ft : math_score;
	s->doc_score = math_score * 100.f + prox_score + fm * ft;
#else /* math only scoring */
	s->math_score = math_score;
	s->text_score = math_score;
	s->doc_score = math_score;
#endif

#ifdef DEBUG_HIT_SCORE_INSPECT
	if (s->doc_score > 0.25f)
//	if (debug_hit_doc == 54047 || debug_hit_doc == 73859 || debug_hit_doc == 85243)
	printf("doc#%u, ft%f, fm%f, max_math%f, prox%f, doc%f\n", debug_hit_doc,
		ft, fm, math_score, prox_score, s->doc_score);
#endif
}

//float
//calc_math_max_score(float doc_score, float tf_idf_score, float prox_score)
//{
//	float math_score = (doc_score - prox_score) / (1.f + tf_idf_score);
//	return math_score * 4.f - 1.f;
//}

static struct rank_hit
*new_hit(doc_id_t hitID, struct overall_scores *s, prox_input_t *prox, int n)
{
	struct rank_hit *hit;
	hit = malloc(sizeof(struct rank_hit));
	hit->docID = hitID;
	hit->math_score = s->math_score;
	hit->text_score = s->text_score;
	hit->score = s->doc_score;
	hit->occurs = malloc(sizeof(hit_occur_t) * MAX_TOTAL_OCCURS);
	hit->n_occurs = prox_sort_occurs(hit->occurs, prox, n);
	return hit;
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
	struct postmerger_postlists root_pols = {0};

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
			root_pols.po[i] = empty_postlist();
		else
			root_pols.po[i] = postmerger_term_postlist(indices, tqs + i);

		/* BM25 per-term global arguments */
		bm25.idf[i] = BM25_idf(&bm25, tqs[i].df);

		root_pols.n += 1;
	}

	/* math score threshold to make into top-K results */
	float theta = 0.f;

	// Insert math posting list iterators
	for (int k = 0; k < qry->len; k++) {
		struct query_keyword *kw = query_keyword(qry, k);

		if (kw->type == QUERY_KEYWORD_TEX) {
			char *kw_str = wstr2mbstr(kw->wstr);
			const int i = root_pols.n;
			const int j = i - sep;
			struct math_qry_struct *ms = mqs + j;

			/* construct level-2 postlist */
			if (0 == math_qry_prepare(indices, kw_str, ms)) {
				mpo[j] = math_l2_postlist(indices, ms, &rk_res, &theta);

				root_pols.po[i] = postmerger_math_l2_postlist(mpo + j);
			} else {
				/* math parse error */
				root_pols.po[i] = empty_postlist();
			}

			root_pols.n += 1;
		}
	}

	// Print processed query
	//(void)math_postlist_cache_list(indices->ci.math_cache, true);
	for (int i = 0; i < root_pols.n; i++) {
		if (i < sep) { /* term query */
			printf("po[%u] `%s' (id=%u, qf=%u, df=%u)\n", i, tqs[i].kw_str,
			       tqs[i].term_id, tqs[i].qf, tqs[i].df);
		} else { /* math query */
			const int j = i - sep;
			printf("po[%u] `%s'\n", i, mqs[j].kw_str);
			if (NULL == root_pols.po[i].po)
				printf("\t[] empty posting list. (parser error)\n");
			else
				math_l2_postlist_print(mpo + j); /* print sub-level posting lists */
		}
	}
	printf("\n");

#ifdef MERGE_TIME_LOG
	// fprintf(mergetime_fh, "checkpoint-init %ld msec.\n", timer_tot_msec(&timer));
#endif

	/* proximity score data structure */
	prox_input_t prox[qry->len];

#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
	uint64_t cnt = 0;
#endif

	/* allocate posting list items storage */
	struct math_l2_postlist_item math_item[qry->n_math];
	struct term_posting_item     term_item[qry->n_term];
	hit_occur_t term_pos[qry->n_term][MAX_TOTAL_OCCURS];

#ifdef SKIP_SEARCH
	goto skip_search;
#endif

#ifdef DEBUG_MATH_MERGE
	printf(ES_RESET_CONSOLE);
	timer_reset(&debug_timer);
#endif

	/* merge top-level posting lists here */
	foreach (iter, postmerger, &root_pols) {
		int h = 0; /* number of hit keywords */

#ifdef PRINT_RECUR_MERGING_ITEMS
		printf("min docID: %u\n", iter->min);
#endif

		/* document length */
#ifndef IGNORE_TERM_INDEX
		uint32_t l = term_index_get_docLen(indices->ti, iter->min);
#else
		uint32_t l = 1;
#endif

		/* calculate term scores */
		float tf_idf_score = 0.f;
		for (int pid = 0; pid < sep; pid++) {

			uint64_t cur = POSTMERGER_POSTLIST_CALL(iter, cur, pid);
			if (cur != UINT64_MAX && cur == iter->min) {
				const int j = pid;
				struct term_posting_item *ti = term_item + j;

				/* read term posting list item */
				POSTMERGER_POSTLIST_CALL(iter, read, pid,
					ti, sizeof(struct term_posting_item));

				/* accumulate term score */
				tf_idf_score += BM25_tf_partial_score(&bm25, j, ti->tf, l);

				/* add term occurs into proximity array */
				P_CAST(ti_occurs, hit_occur_t, term_pos + j);
				for (int k = 0; k < ti->n_occur; k++)
					ti_occurs[k].pos = ti->pos_arr[k];
				prox_set_input(prox + (h++), ti_occurs, ti->n_occur);

#ifdef PRINT_RECUR_MERGING_ITEMS
				printf("hit term po[%u]: ", pid);
				printf("%s=%u, ", STRVAR_PAIR(ti->doc_id));
				printf("%s=%u, ", STRVAR_PAIR(ti->tf));
				printf("%s=%u: ", STRVAR_PAIR(ti->n_occur));
				for (int k = 0; k < ti->n_occur; k++)
					printf("%u ", ti->pos_arr[k]);
				printf("\n");
#endif
				/* advance posting iterators */
				POSTMERGER_POSTLIST_CALL(iter, next, pid);
			}
		}

		/* calculate (term) proximity score */
		float prox_score = 0.f;
#ifdef ENABLE_PROXIMITY_SCORE
		if (h > 0)
			prox_score = proximity_score(prox, h);
#endif

#if 0
		/* calculate math threshold score inversely given term scores. */
		if (h > 0) {
			float doc_score = 0.f;
			if (priority_Q_full(&rk_res))
				doc_score = priority_Q_min_score(&rk_res);
			theta = calc_math_max_score(doc_score, tf_idf_score, prox_score);
		} else {
			theta = 0.f;
		}
#endif

		/* calculate math scores */
		float math_score = 0.f;
		for (int pid = sep; pid < root_pols.n; pid++) {
#ifdef DEBUG_MATH_MERGE
			if (debug_print)
				math_l2_postlist_print_merge_state(iter->po[pid].po);
#endif
			uint64_t cur = POSTMERGER_POSTLIST_CALL(iter, cur, pid);
			if (cur != UINT64_MAX && cur == iter->min) {
				const int j = pid - sep;
				struct math_l2_postlist_item *mi = math_item + j;

				POSTMERGER_POSTLIST_CALL(iter, read, pid,
					mi, sizeof(struct math_l2_postlist_item));

				/* find math score from maximum math part score */
				math_score += mi->part_score;

				prox_set_input(prox + (h++), mi->occurs, mi->n_occurs);

#ifdef PRINT_RECUR_MERGING_ITEMS
				printf("hit math po[%u]: ", pid);
				printf("%s=%u, ",   STRVAR_PAIR(mi->doc_id));
				printf("%s=%.3f, ", STRVAR_PAIR(mi->part_score));
				printf("%s=%u: ",   STRVAR_PAIR(mi->n_occurs));
				for (int k = 0; k < mi->n_occurs; k++)
					printf("%u ", mi->occurs[k].pos);
				printf("\n");
#endif
				/* advance posting iterators */
				POSTMERGER_POSTLIST_CALL(iter, next, pid);
			}
		}

#ifdef PRINT_RECUR_MERGING_ITEMS
		printf("\n");
#endif

		if (h > 0) {
			doc_id_t hit_doc = (doc_id_t)iter->min;
#ifdef DEBUG_HIT_SCORE_INSPECT
			debug_hit_doc = hit_doc;
#endif
			/* now, calculate the overall score for ranking */
			struct overall_scores s;
			calc_overall_scores(&s, math_score, tf_idf_score, prox_score);

			prox_reset_inputs(prox, h); /* reset */

			/* consider as top-K candidate */
			if (!priority_Q_full(&rk_res) ||
			    s.doc_score > priority_Q_min_score(&rk_res)) {
				struct rank_hit *hit = new_hit(hit_doc, &s, prox, h);
				priority_Q_add_or_replace(&rk_res, hit);
			}
		}

#ifdef DEBUG_MATH_MERGE
		long msec = timer_tot_msec(&debug_timer);
		debug_print = 0;
		if (msec > 100 /* sample every 100 msec */) {
			printf(ES_RESET_CONSOLE);
			timer_reset(&debug_timer);
			debug_print = 1;
		}
		if (debug_slowdown) delay(3, 0, 0); else delay(0, 0, 1);
#endif

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

	// Destroy allocated posting lists
	for (int i = 0; i < root_pols.n; i++) {
		/* free math query structure */
		if (i >= sep)
			math_qry_free(mqs + (i - sep));

		if (NULL == root_pols.po[i].po)
			continue;

		if (i < sep) /* free term postlist */
			term_postlist_free(root_pols.po[i]);
		else /* free math level 2 postlist */
			math_l2_postlist_free(root_pols.po[i]);
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
