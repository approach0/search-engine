#pragma once
#include "indices/indices.h"
#include "postmerge/postmerger.h"

#include "rank.h"
#include "snippet.h"
#include "query.h"
#include "math-qry-struct.h"

struct math_l2_postlist {
	struct postmerger pm;
	postmerger_iter_t iter;

	char type[MAX_MERGE_POSTINGS][128];
	int  weight[MAX_MERGE_POSTINGS];
	struct subpath_ele *ele[MAX_MERGE_POSTINGS];

	struct math_qry_struct *mqs;
	struct indices *indices /* for debug */;
};

ranked_results_t
indices_run_query(struct indices*, struct query*);
