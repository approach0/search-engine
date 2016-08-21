#include <stdio.h>
#include <stdlib.h>

#include "mem-index/mem-posting.h"

#include "config.h"
#include "postmerge.h"
#include "search.h"
#include "search-utils.h"
#include "math-expr-search.h"
#include "math-search.h"

#pragma pack(push, 1)
typedef struct {
	/* consistent variables */
	struct postmerge   *top_pm;
	enum query_kw_type *kw_type;

	/* writing posting list */
	struct mem_posting *wr_mem_po;
	uint32_t            last_visits;

	/* statical variables */
	uint32_t            n_mem_po;
	float               mem_cost;

	/* document math score item buffer */
	math_score_posting_item_t last;
	position_t reserve[MAX_HIGHLIGHT_OCCURS];

} math_score_combine_args_t;
#pragma pack(pop)

static void
msca_push_pos(math_score_combine_args_t *msca, position_t pos)
{
	uint32_t *i = &msca->last.n_match;
	if (*i < MAX_HIGHLIGHT_OCCURS) {
		msca->last.pos_arr[*i] = pos;
		(*i) ++;
	}
}

static void
msca_rst(math_score_combine_args_t *msca, doc_id_t docID)
{
	msca->last.docID   = docID;
	msca->last.score   = 0;
	msca->last.n_match = 0;
}

/* debug print function */
static void print_math_score_posting(struct mem_posting*);

static void
write_math_score_posting(math_score_combine_args_t *msca)
{
	size_t wr_sz = sizeof(math_score_posting_item_t);
	math_score_posting_item_t *mip = &msca->last;

	if (mip->score > 0) {
		wr_sz += mip->n_match * sizeof(position_t);
		mem_posting_write(msca->wr_mem_po, mip, wr_sz);
	}
}

static void
add_math_score_posting(math_score_combine_args_t *msca)
{
	struct postmerge_callbks *pm_calls;

	/* flush write */
	write_math_score_posting(msca);
	mem_posting_write_complete(msca->wr_mem_po);

#ifdef DEBUG_MATH_SCORE_POSTING
	printf("\n");
	printf("adding math-score posting list:\n");
	print_math_score_posting(msca->wr_mem_po);
	printf("\n");
#endif

	/* record memory cost increase */
	msca->mem_cost += (float)msca->wr_mem_po->tot_sz;

	/* add this posting list to top level postmerge */
	pm_calls = get_memory_postmerge_callbks();
	postmerge_posts_add(msca->top_pm, msca->wr_mem_po,
	                    pm_calls, msca->kw_type);
}

static void
math_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	struct math_expr_score_res res;
	P_CAST(mesa, struct math_extra_score_arg, extra_args);
	P_CAST(msca, math_score_combine_args_t, mesa->expr_srch_arg);

	/* calculate expression similarity on merge */
	res = math_expr_score_on_merge(pm, mesa->dir_merge_level,
	                               mesa->n_qry_lr_paths);

#ifdef DEBUG_MATH_SEARCH
	printf("\n");
	printf("directory visited: %u\n", mesa->n_dir_visits);
	printf("visting depth: %u\n", mesa->dir_merge_level);
#endif

	/*
	 * check whether we are reaching the limit of total number
	 * of merging posting lists. If yes, force stop traverse
	 * the next directory after this posting merge is done.
	 */
	if (msca->top_pm->n_postings + 1 >= MAX_MERGE_POSTINGS) {
		/* remember to stop traversing the next directory */
		mesa->stop_dir_search = 1;

		/*
		 * prevent creating a new posting list during this
		 * posting merge.
		 */
		if (msca->wr_mem_po == NULL)
			return;
	}

	/*
	 * first, checkout to a new posting list when a new
	 * directory is visited.
	 */
	if (msca->wr_mem_po == NULL ||
	    mesa->n_dir_visits != msca->last_visits) {

		/* if posting list is not NULL, add it */
		if (msca->wr_mem_po)
			add_math_score_posting(msca);

		/* reset and create new posting list */
		msca_rst(msca, 0);
		msca->wr_mem_po = mem_posting_create(
			DEFAULT_SKIPPY_SPANS,
			math_score_posting_plain_calls()
		);
		msca->n_mem_po ++;

		/* update last_visits */
		msca->last_visits = mesa->n_dir_visits;
	}

	/*
	 * second, write last posting list item when this
	 * expression is a new document ID.
	 */
	if (res.doc_id != msca->last.docID) {

		write_math_score_posting(msca);
		msca_rst(msca, res.doc_id);
	}

	/* finally, update current maximum expression score
	 * inside of current evaluating document, also update
	 * expression positions of current document */
	if (res.score > msca->last.score)
		msca->last.score = res.score;

	msca_push_pos(msca, res.exp_id);
}

uint32_t
add_math_postinglist(struct postmerge *pm, struct indices *indices,
                     char *kw_utf8, enum query_kw_type *kw_type)
{
	math_score_combine_args_t msca;

	/* initialize score combine arguments */
	msca.top_pm      = pm;
	msca.kw_type     = kw_type;
	msca.wr_mem_po   = NULL;
	msca.last_visits = 0;
	msca.n_mem_po    = 0;
	msca.mem_cost    = 0.f;
	msca_rst(&msca, 0);

	/* merge and combine math scores */
	math_expr_search(indices->mi, kw_utf8, DIR_MERGE_DEPTH_FIRST,
	                 &math_posting_on_merge, &msca);

	if (msca.wr_mem_po)
		/* flush and final adding */
		add_math_score_posting(&msca);
	else
		/* add a NULL posting to indicate empty result */
		postmerge_posts_add(msca.top_pm, NULL, NULL, kw_type);

#ifdef VERBOSE_SEARCH
	printf("`%s' has %u math score posting(s) (%f KB).\n",
	       kw_utf8, msca.n_mem_po, msca.mem_cost / 1024.f);
#endif

	return msca.n_mem_po;
}

static void print_math_score_posting(struct mem_posting *po)
{
	math_score_posting_item_t *mip;
	if (!mem_posting_start(po)) {
		printf("(empty)");
		return;
	}

	do {
		mip = mem_posting_cur_item(po);
		printf("[docID=%u, score=%u, n_match=%u", mip->docID,
			   mip->score, mip->n_match);

		if (mip->n_match != 0) {
			uint32_t i;
			printf(": ");
			for (i = 0; i < mip->n_match; i++)
				printf("%u ", mip->pos_arr[i]);
		}

		printf("]");
	} while (mem_posting_next(po));

	mem_posting_finish(po);
}
