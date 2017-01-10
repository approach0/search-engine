#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#undef NDEBUG
#include <assert.h>

#include "wstring/wstring.h" /* for wstr2mbstr() */
#include "mem-index/mem-posting.h"
#include "timer/timer.h"

#include "config.h"
#include "postmerge.h"
#include "bm25-score.h"
#include "proximity.h"
#include "search.h"
#include "search-utils.h"
#include "math-search.h"

struct adding_post_arg {
	struct indices          *indices;
	struct postmerge        *pm;
	uint32_t                 docN;
	uint32_t                 idx;
	float                   *idf;
};

static bool
add_term_postinglist(struct postmerge *pm, struct indices *indices,
                     char *kw_utf8, enum query_kw_type *kw_type)
{
	void *post;
	struct postmerge_callbks *pm_calls;

	bool ret;
	void *ti;
	term_id_t term_id;

#ifdef VERBOSE_SEARCH
	struct timer timer;

	/* start timer */
	timer_reset(&timer);
#endif

	/* some short-hand variables */
	ti = indices->ti;
	term_id = term_lookup(ti, kw_utf8);

	if (term_id == 0) {
		/* if term is not found in our dictionary */
		post = NULL;
		pm_calls = NULL;

#ifdef VERBOSE_SEARCH
		printf("`%s' has empty posting list.\n", kw_utf8);
#endif
		ret = 0;
	} else {
		/* otherwise, get on-disk or cached posting list */
		struct postcache_item *cache_item =
			postcache_find(&indices->postcache, term_id);

		if (NULL != cache_item) {
			/* if this term is already cached */
			post = cache_item->posting;
			pm_calls = get_memory_postmerge_callbks();

#ifdef VERBOSE_SEARCH
		printf("`%s' uses cached posting list.\n", kw_utf8);
#endif
		} else {
			/* otherwise read posting from disk */
			post = term_index_get_posting(ti, term_id);
			pm_calls = get_disk_postmerge_callbks();
#ifdef VERBOSE_SEARCH
		printf("`%s' uses on-disk posting list.\n", kw_utf8);
#endif
		}

		ret = 1;
	}

	/* add posting list for merge */
	postmerge_posts_add(pm, post, pm_calls, kw_type);

#ifdef VERBOSE_SEARCH
	printf("term-post adding time cost: %ld msec.\n",
	       timer_tot_msec(&timer));
#endif
	return ret;
}

static LIST_IT_CALLBK(add_postinglist)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(aa, struct adding_post_arg, pa_extra);
	char *kw_utf8 = wstr2mbstr(kw->wstr);
	enum query_kw_type *kw_type = &kw->type;
	float docN;
	uint32_t n;

	switch (kw->type) {
	case QUERY_KEYWORD_TERM:
		docN = (float)aa->docN;

		if (add_term_postinglist(aa->pm, aa->indices, kw_utf8, kw_type))
			aa->idf[aa->idx] = BM25_idf(kw->df, docN);
		else
			aa->idf[aa->idx] = BM25_idf(0, docN);

#ifdef VERBOSE_SEARCH
		printf("posting[%u].\n", aa->idx);
#endif
		aa->idx ++;
		break;

	case QUERY_KEYWORD_TEX:
		n = add_math_postinglist(aa->pm, aa->indices, kw_utf8, kw_type);
#ifdef VERBOSE_SEARCH
//		{
//			int i;
//			for (i = aa->idx; i < aa->idx + n; i++) {
//				printf("posting[%u]", i);
//				if (i + 1 == n)
//					printf(".");
//				else
//					printf(", ");
//			}
//			printf("\n");
//		}
#endif
		aa->idx += n;
		break;

	default:
		assert(0);
	}

	if (aa->idx < MAX_MERGE_POSTINGS) {
		LIST_GO_OVER;
	} else {
		return LIST_RET_BREAK;
	}
}

static uint32_t
add_postinglists(struct indices *indices, const struct query *qry,
                 struct postmerge *pm, float *idf_arr)
{
	/* setup argument variable `ap_args' (in & out) */
	struct adding_post_arg ap_args;
	ap_args.indices = indices;
	ap_args.pm = pm;
	ap_args.docN = term_index_get_docN(indices->ti);
	ap_args.idx = 0;
	ap_args.idf = idf_arr;

	/* add posting list of every keyword for merge */
	list_foreach((list*)&qry->keywords, &add_postinglist, &ap_args);

	return ap_args.idx;
}

static void
mixed_posting_on_merge(uint64_t cur_min, struct postmerge *pm,
                       void *extra_args)
{
	P_CAST(pm_args, struct posting_merge_extra_args, extra_args);
	uint32_t    i, j = 0;

	float       tot_score;
	float       math_score, bm25_score = 1.f;
	mnc_score_t max_math_score = 0;
	position_t *pos_arr;

#ifdef ENABLE_PROXIMITY_SCORE
	float       prox_score;
	position_t  minDist;
#endif

	doc_id_t    docID = cur_min;
	float       doclen;

	doclen = (float)term_index_get_docLen(pm_args->indices->ti, docID);

	enum query_kw_type        *type;
	struct term_posting_item  *pip;
	math_score_posting_item_t *mip;

	for (i = 0; i < pm->n_postings; i++)
		if (pm->curIDs[i] == cur_min) {
			type = (enum query_kw_type *)pm->posting_args[i];

			switch (*type) {
			case QUERY_KEYWORD_TERM:
				pip = pm->cur_pos_item[i];
				bm25_score += BM25_term_i_score(
					pm_args->bm25args,
					i, pip->tf, doclen
				);

				/* set proximity input */
				pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);
				prox_set_input(pm_args->prox_in + j, pos_arr, pip->tf);
				j++;

				break;

			case QUERY_KEYWORD_TEX:
				mip = pm->cur_pos_item[i];

				if (mip->score > max_math_score)
					max_math_score = mip->score;

				/* set proximity input */
				pos_arr = mip->pos_arr;
				prox_set_input(pm_args->prox_in + j, pos_arr, mip->n_match);
				j++;

				break;

			default:
				assert(0);
			}
		}

#ifdef ENABLE_PROXIMITY_SCORE
	/* calculate overall score considering proximity. */
	minDist = prox_min_dist(pm_args->prox_in, j);
	prox_score = prox_calc_score(minDist);

	/* reset prox_in for consider_top_K() function */
	prox_reset_inputs(pm_args->prox_in, j);
#endif

	/*
	 * math score of a document is determined by the max
	 * scored expression that occurs in this document.
	 */
	math_score = 1.f + (float)max_math_score;
	math_score = math_score / 2.f;

//	printf("doc#%u, prox_score %f, math score %f, bm25 score %f.\n",
//	       docID, prox_score, math_score, bm25_score);

	tot_score = prox_score + math_score * bm25_score;

	consider_top_K(pm_args->rk_res, docID, tot_score,
	               pm_args->prox_in, j);
}

static void free_math_postinglists(struct postmerge *pm)
{
	uint32_t i;
	enum query_kw_type *type;
	struct mem_posting *mem_po;

	for (i = 0; i < pm->n_postings; i++) {
		mem_po = (struct mem_posting*)pm->postings[i];
		type = (enum query_kw_type *)pm->posting_args[i];

		if (*type == QUERY_KEYWORD_TEX && mem_po != NULL) {
			//printf("releasing math in-memory posting[%u]...\n", i);

			mem_posting_free(mem_po);
		}
	}
}

ranked_results_t
indices_run_query(struct indices *indices, struct query *qry)
{
	struct postmerge                pm;
	struct BM25_term_i_args         bm25args;
	ranked_results_t                rk_res;
	struct posting_merge_extra_args pm_args;
	uint32_t                        n_add;

#ifdef VERBOSE_SEARCH
	struct timer timer;

	/* start timer */
	timer_reset(&timer);
#endif

	/*
	 * some query pre-merge process.
	 */
	set_keywords_val(qry, indices);

	/* sort query, to prioritize keywords in highlight stage */
	query_sort_by_df(qry);

	/* make query unique by post_id, avoid mem-posting overlap */
	query_uniq_by_post_id(qry);

#ifdef VERBOSE_SEARCH
	printf("\n");
	printf("processed query: \n");
	query_print_to(*qry, stdout);
	printf("\n\n");
#endif

	/* initialize postmerge */
	postmerge_posts_clear(&pm);

	n_add = add_postinglists(indices, qry, &pm,
	                         (float*)&bm25args.idf);
#ifdef VERBOSE_SEARCH
	printf("\n");
	printf("post-adding total time cost: %ld msec.\n",
	       timer_last_msec(&timer));

	printf("%u posting lists added.\n", n_add);
	printf("\n");
#else
	n_add++; /* supress compile warning */
#endif

	/* prepare BM25 parameters */
	bm25args.n_postings = pm.n_postings;
	bm25args.avgDocLen = (float)term_index_get_avgDocLen(indices->ti);
	bm25args.b  = BM25_DEFAULT_B;
	bm25args.k1 = BM25_DEFAULT_K1;
	bm25args.frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25args.avgDocLen;

#ifdef VERBOSE_SEARCH
//	printf("BM25 arguments:\n");
//	BM25_term_i_args_print(&bm25args);
//	printf("\n");
#endif

	/* initialize ranking queue */
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

	/* setup merge extra arguments */
	pm_args.indices  = indices;
	pm_args.bm25args = &bm25args;
	pm_args.rk_res   = &rk_res;
	pm_args.prox_in  = malloc(sizeof(prox_input_t) * pm.n_postings);

	/* posting list merge */
#ifdef VERBOSE_SEARCH
	printf("start merging...\n");
#endif
	if (!posting_merge(&pm, POSTMERGE_OP_OR,
	                   &mixed_posting_on_merge, &pm_args))
		fprintf(stderr, "posting list merge failed.\n");

#ifdef VERBOSE_SEARCH
	printf("top-level merge time cost: %ld msec.\n",
	       timer_last_msec(&timer));

	printf("start ranking...\n");
#endif
	/* free proximity pointer array */
	free(pm_args.prox_in);

	/* free temporal math posting lists */
	free_math_postinglists(&pm);

	/* rank top K hits */
	priority_Q_sort(&rk_res);

#ifdef VERBOSE_SEARCH
	/* report search time cost */
	printf("search time cost: %ld msec.\n",
	       timer_tot_msec(&timer));
#endif

	/* return top K hits */
	return rk_res;
}
