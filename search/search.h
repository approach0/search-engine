#pragma once
#include "indices/indices.h"
#include "postmerge/postmerger.h"

#include "config.h"
#include "rank.h"
#include "snippet.h"
#include "query.h"
#include "math-qry-struct.h"
#include "math-pruning.h"

struct math_l2_postlist {
	struct postmerger pm; /* posting lists */
	postmerger_iter_t iter;

	char type[MAX_MERGE_POSTINGS][128];
	int  weight[MAX_MERGE_POSTINGS];
	struct subpath_ele *ele[MAX_MERGE_POSTINGS];

	struct math_qry_struct *mqs;
	struct indices *indices /* for debug */;

	uint32_t    prev_doc_id;
	uint32_t    max_exp_score;
	uint32_t    n_occurs;
	hit_occur_t occurs[MAX_HIGHLIGHT_OCCURS];

	struct math_pruner pruner;
};

struct l2_postlist_item {
	uint32_t    doc_id;
	uint32_t    part_score;
	uint32_t    n_occurs;
	hit_occur_t occurs[MAX_HIGHLIGHT_OCCURS];
};

ranked_results_t
indices_run_query(struct indices*, struct query*);
