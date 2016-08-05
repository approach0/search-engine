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
	/* output memory posting list */
	struct mem_posting       *mem_po;

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

static void
write_math_score_posting(math_score_combine_args_t *msca)
{
	size_t wr_sz = sizeof(math_score_posting_item_t);
	math_score_posting_item_t *mip = &msca->last;

	if (mip->score > 0) {
		wr_sz += mip->n_match * sizeof(position_t);
		mem_posting_write(msca->mem_po, mip, wr_sz);
	}
}

static void
math_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	struct math_expr_score_res res;
	P_CAST(mesa, struct math_extra_score_arg, extra_args);
	P_CAST(msca, math_score_combine_args_t, mesa->extra_search_args);

	/* calculate expression similarity on merge */
	res = math_expr_score_on_merge(pm, mesa->dir_merge_level,
	                               mesa->n_qry_lr_paths);

	if (res.doc_id != msca->last.docID) {
		/* this expression is in a new document */

		write_math_score_posting(msca);

		/* clear msca */
		msca_rst(msca, res.doc_id);
	}

	/* accumulate score and positions of current document */
	msca->last.score += res.score;
	msca_push_pos(msca, res.exp_id);
}

/* debug print function */
static void print_math_score_posting(struct mem_posting*);

void
add_math_postinglist(struct postmerge *pm, struct indices *indices,
                     char *kw_utf8, enum query_kw_type *kw_type)
{
	struct postmerge_callbks *pm_calls;
	math_score_combine_args_t msca;
	struct mem_posting       *mempost;

	/* create memory posting list for math intermediate results */
	mempost = mem_posting_create(DEFAULT_SKIPPY_SPANS,
	                             math_score_posting_plain_calls());

	/* initialize score combine arguments */
	msca.mem_po = mempost;
	msca_rst(&msca, 0);

	/* merge and combine math scores */
	math_expr_search(indices->mi, kw_utf8, DIR_MERGE_DEPTH_FIRST,
	                 &math_posting_on_merge, &msca);

	/* flush write */
	write_math_score_posting(&msca);
	mem_posting_write_complete(mempost);

#ifdef VERBOSE_SEARCH
	printf("(math score posting, %f KB)",
	       (float)mempost->tot_sz / 1024.f);
#endif

#ifdef DEBUG_MATH_SCORE_POSTING
	print_math_score_posting(mempost);
#endif

	/* add math score (in-memory) posting list for merge */
	pm_calls = get_memory_postmerge_callbks();
	postmerge_posts_add(pm, mempost, pm_calls, kw_type);
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
