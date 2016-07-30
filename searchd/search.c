#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#undef NDEBUG
#include <assert.h>

#include "wstring/wstring.h"
#include "mem-index/mem-posting.h"

#include "config.h"
#include "postmerge.h"
#include "bm25-score.h"
#include "proximity.h"
#include "math-search.h"
#include "search.h"
#include "search-utils.h"

struct add_postinglist_arg {
	struct indices          *indices;
	struct postmerge        *pm;
	uint32_t                 docN;
	uint32_t                 posting_idx;
	float                   *idf;
};

struct add_postinglist {
	void *posting;
	struct postmerge_callbks *postmerge_callbks;
};

static struct add_postinglist
term_postinglist(char *kw_utf8, float df, struct add_postinglist_arg *ap_args)
{
	struct add_postinglist ret;
	void                  *ti;
	term_id_t              term_id;
	float                  docN;

	/* get variables for short-hand */
	ti = ap_args->indices->ti;
	term_id = term_lookup(ti, kw_utf8);
	docN = (float)ap_args->docN;

	printf("posting list[%u] of keyword `%s'(termID: %u)...\n",
	       ap_args->posting_idx, kw_utf8, term_id);

	if (term_id == 0) {
		/* if term is not found in our dictionary */
		ret.posting = NULL;
		ret.postmerge_callbks = NULL;

		/* set BM25 idf argument */
		ap_args->idf[ap_args->posting_idx] = BM25_idf(0.f, docN);

		printf("keyword not found.\n");
	} else {
		/* otherwise, get on-disk or cached posting list */
		struct postcache_item *cache_item =
			postcache_find(&ap_args->indices->postcache, term_id);

		if (NULL != cache_item) {
			/* if this term is already cached */
			ret.posting = cache_item->posting;
			ret.postmerge_callbks = get_memory_postmerge_callbks();

			printf("using cached posting list.\n");
		} else {
			/* otherwise read posting from disk */
			ret.posting = term_index_get_posting(ti, term_id);
			ret.postmerge_callbks = get_disk_postmerge_callbks();
			printf("using on-disk posting list.\n");
		}

		/* set BM25 idf argument */
		ap_args->idf[ap_args->posting_idx] = BM25_idf(df, docN);
	}

	return ret;
}

static LIST_IT_CALLBK(add_postinglist)
{
	struct add_postinglist res;
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(ap_args, struct add_postinglist_arg, pa_extra);
	char *kw_utf8 = wstr2mbstr(kw->wstr);

	switch (kw->type) {
	case QUERY_KEYWORD_TERM:
		res = term_postinglist(kw_utf8, kw->df, ap_args);
		break;

	case QUERY_KEYWORD_TEX:
		//res = math_postinglist(kw_utf8, ap_args);
		break;

	default:
		assert(0);
	}

	/* add posting list for merge */
	postmerge_posts_add(ap_args->pm, res.posting,
	                    res.postmerge_callbks, &kw->type);

	/* increment current adding posting list index */
	ap_args->posting_idx ++;

	LIST_GO_OVER;
}

static uint32_t
add_postinglists(struct indices *indices, const struct query *qry,
                 struct postmerge *pm, float *idf_arr)
{

	/* setup argument variable `ap_args' (in & out) */
	struct add_postinglist_arg ap_args;
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
	uint32_t    i;
	float       mnc_score, bm25_score, tot_score = 0.f;
#ifdef ENABLE_PROXIMITY_SEARCH
	uint32_t    j = 0;
	float       prox_score;
	position_t *pos_arr;
	position_t  minDist;
#endif
	doc_id_t    docID = cur_min;
	uint32_t    n_tot_occurs = 0;
	float       doclen = (float)term_index_get_docLen(pm_args->indices->ti, docID);

	enum query_kw_type       *type;
	struct term_posting_item *pip;
	//struct math_doc_score  *mds;

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

#ifdef ENABLE_PROXIMITY_SEARCH
				/* set proximity input */
				pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);
				prox_set_input(pm_args->prox_in + j, pos_arr, pip->tf);
				j++;
#endif

				break;

			case QUERY_KEYWORD_TEX:
				n_tot_occurs += 0;
				mnc_score = 0;
				tot_score += mnc_score;

				break;

			default:
				assert(0);
			}
		}

#ifdef ENABLE_PROXIMITY_SEARCH
	/* calculate overall score considering proximity. */
	minDist = prox_min_dist(pm_args->prox_in, j);
	prox_score = prox_calc_score(minDist);
	tot_score += prox_score;
#endif

	consider_top_K(pm_args->rk_res, docID, tot_score, pm, n_tot_occurs);
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
			printf("releasing math in-memory posting[%u]...\n",
			       i);

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
	uint32_t                        n_added;

	/* sort query, to prioritize keywords in highlight stage */
	printf("query before sorting:\n");
	query_print_to(qry, stdout);

	list_foreach((list*)&qry.keywords, &set_df_value, indices);
	query_sort(&qry);

	printf("query after sorting:\n");
	query_print_to(qry, stdout);

	/* initialize postmerge */
	postmerge_posts_clear(&pm);
	n_added = add_postinglists(indices, &qry, &pm, (float*)&bm25args.idf);

	/* prepare BM25 parameters */
	bm25args.n_postings = n_added;
	bm25args.avgDocLen = (float)term_index_get_avgDocLen(indices->ti);
	bm25args.b  = BM25_DEFAULT_B;
	bm25args.k1 = BM25_DEFAULT_K1;
	bm25args.frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25args.avgDocLen;

	printf("BM25 arguments:\n");
	BM25_term_i_args_print(&bm25args);

	/* initialize ranking queue */
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

	/* setup merge extra arguments */
	pm_args.indices  = indices;
	pm_args.bm25args = &bm25args;
	pm_args.rk_res   = &rk_res;
	pm_args.prox_in  = malloc(sizeof(prox_input_t) * pm.n_postings);

	/* posting list merge */
	printf("start merging...\n");
	if (!posting_merge(&pm, POSTMERGE_OP_OR,
	                   &mixed_posting_on_merge, &pm_args))
		fprintf(stderr, "posting list merge failed.\n");

	/* free proximity pointer array */
	free(pm_args.prox_in);

	/* free temporal math posting lists */
	free_math_postinglists(&pm);

	/* rank top K hits */
	priority_Q_sort(&rk_res);

	/* return top K hits */
	return rk_res;
}
