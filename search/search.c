#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#undef NDEBUG
#include <assert.h>

#include "wstring/wstring.h"
#include "mem-index/mem-posting.h"
#include "indexer/index.h" /* for eng_to_lower_case() */

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
	uint32_t                 posting_idx;
	float                   *idf;
};

static void
add_term_postinglist(struct postmerge *pm, struct indices *indices,
                     char *kw_utf8, enum query_kw_type *kw_type,
                     uint32_t docN_, float df, float *idf, uint32_t i)
{
	void                     *post;
	struct postmerge_callbks *pm_calls;

	void      *ti;
	term_id_t  term_id;
	float      docN;

	/* some short-hand variables */
	ti = indices->ti;
	term_id = term_lookup(ti, kw_utf8);
	docN = (float)docN_;

	if (term_id == 0) {
		/* if term is not found in our dictionary */
		post = NULL;
		pm_calls = NULL;

		/* set BM25 idf argument */
		idf[i] = BM25_idf(0.f, docN);

#ifdef VERBOSE_SEARCH
		printf("(empty posting)");
#endif
	} else {
		/* otherwise, get on-disk or cached posting list */
		struct postcache_item *cache_item =
			postcache_find(&indices->postcache, term_id);

		if (NULL != cache_item) {
			/* if this term is already cached */
			post = cache_item->posting;
			pm_calls = get_memory_postmerge_callbks();

#ifdef VERBOSE_SEARCH
			printf("(cached in memory)");
#endif
		} else {
			/* otherwise read posting from disk */
			post = term_index_get_posting(ti, term_id);
			pm_calls = get_disk_postmerge_callbks();
#ifdef VERBOSE_SEARCH
			printf("(on disk)");
#endif
		}

		/* set BM25 idf argument */
		idf[i] = BM25_idf(df, docN);
	}

	/* add posting list for merge */
	postmerge_posts_add(pm, post, pm_calls, kw_type);
}

static LIST_IT_CALLBK(add_postinglist)
{
	/* posting list and merge callbacks to be added */

	/* castings */
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(aa, struct adding_post_arg, pa_extra);
	char *kw_utf8 = wstr2mbstr(kw->wstr);
	enum query_kw_type *kw_type = &kw->type;

#ifdef VERBOSE_SEARCH
	printf("posting list[%u]: `%s' ", aa->posting_idx, kw_utf8);
#endif

	switch (kw->type) {
	case QUERY_KEYWORD_TERM:
		eng_to_lower_case(kw_utf8, strlen(kw_utf8));
		add_term_postinglist(aa->pm, aa->indices, kw_utf8, kw_type,
		                     aa->docN, kw->df, aa->idf, aa->posting_idx);
		break;

	case QUERY_KEYWORD_TEX:
		add_math_postinglist(aa->pm, aa->indices, kw_utf8, kw_type);
		break;

	default:
		assert(0);
	}

#ifdef VERBOSE_SEARCH
	printf("\n");
#endif

	/* increment current adding posting list index */
	aa->posting_idx ++;

	LIST_GO_OVER;
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
	ap_args.posting_idx = 0;
	ap_args.idf = idf_arr;

	/* add posting list of every keyword for merge */
	list_foreach((list*)&qry->keywords, &add_postinglist, &ap_args);

	return ap_args.posting_idx;
}

static void
mixed_posting_on_merge(uint64_t cur_min, struct postmerge *pm,
                       void *extra_args)
{
	P_CAST(pm_args, struct posting_merge_extra_args, extra_args);
	uint32_t    i, j = 0;
	float       math_score, bm25_score, tot_score = 0.f;
	position_t *pos_arr;

#ifdef ENABLE_PROXIMITY_SCORE
	float       prox_score;
	position_t  minDist;
#endif

	doc_id_t    docID = cur_min;
	uint32_t    n_tot_occurs = 0;
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
				n_tot_occurs += pip->tf;
				bm25_score = BM25_term_i_score(
					pm_args->bm25args,
					i, pip->tf, doclen
				);

				tot_score += bm25_score;

				/* set proximity input */
				pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);
				prox_set_input(pm_args->prox_in + j, pos_arr, pip->tf);
				j++;

				break;

			case QUERY_KEYWORD_TEX:
				mip = pm->cur_pos_item[i];
				n_tot_occurs += mip->n_match;
				math_score = (float)mip->score;
				tot_score += math_score;

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
	tot_score += prox_score;
#endif

	consider_top_K(pm_args->rk_res, docID, tot_score,
	               pm_args->prox_in, j, n_tot_occurs);
	return;
}

static void free_math_postinglists(struct postmerge *pm)
{
	uint32_t i;
	enum query_kw_type *type;
	struct mem_posting *mem_po;

	for (i = 0; i < pm->n_postings; i++) {
		mem_po = (struct mem_posting*)pm->postings[i];
		type = (enum query_kw_type *)pm->posting_args[i];

		if (*type == QUERY_KEYWORD_TEX) {
			//printf("releasing math in-memory posting[%u]...\n", i);

			mem_posting_free(mem_po);
		}
	}
}

static LIST_IT_CALLBK(set_df_value)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(indices, struct indices, pa_extra);
	term_id_t term_id;

	term_id = term_lookup(indices->ti, wstr2mbstr(kw->wstr));

	if (term_id == 0)
		kw->df = 0;
	else
		kw->df = (uint64_t)term_index_get_df(indices->ti, term_id);

	LIST_GO_OVER;
}

ranked_results_t
indices_run_query(struct indices *indices, const struct query qry)
{
	struct postmerge                pm;
	struct BM25_term_i_args         bm25args;
	ranked_results_t                rk_res;
	struct posting_merge_extra_args pm_args;
	uint32_t                        n_postings;

	/* sort query, to prioritize keywords in highlight stage */
	list_foreach((list*)&qry.keywords, &set_df_value, indices);
	query_sort(&qry);

#ifdef VERBOSE_SEARCH
	printf("sorted query:\n");
	query_print_to(qry, stdout);
	printf("\n");
#endif

	/* initialize postmerge */
	postmerge_posts_clear(&pm);
	n_postings = add_postinglists(indices, &qry, &pm, (float*)&bm25args.idf);
#ifdef VERBOSE_SEARCH
	printf("\n");
#endif

	/* prepare BM25 parameters */
	bm25args.n_postings = n_postings;
	bm25args.avgDocLen = (float)term_index_get_avgDocLen(indices->ti);
	bm25args.b  = BM25_DEFAULT_B;
	bm25args.k1 = BM25_DEFAULT_K1;
	bm25args.frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25args.avgDocLen;

#ifdef VERBOSE_SEARCH
	printf("BM25 arguments:\n");
	BM25_term_i_args_print(&bm25args);
	printf("\n");
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
	printf("start ranking...\n");
#endif
	/* free proximity pointer array */
	free(pm_args.prox_in);

	/* free temporal math posting lists */
	free_math_postinglists(&pm);

	/* rank top K hits */
	priority_Q_sort(&rk_res);

	/* return top K hits */
	return rk_res;
}
