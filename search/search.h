#pragma once
#include "indices/indices.h"
#include "postmerge/postmerger.h"
#include "hashtable/u16-ht.h"

#include "config.h"
#include "rank.h"
#include "snippet.h"
#include "query.h"
#include "math-qry-struct.h"
#include "math-pruning.h"

struct math_l2_postlist {
	struct postmerger pm; /* posting lists */
	postmerger_iter_t iter;

	struct subpath_ele *ele[MAX_MERGE_POSTINGS];

	struct math_qry_struct *mqs;
	struct indices *indices /* for debug */;
	ranked_results_t *rk_res;

	/* current doc-level item recorder */
	uint32_t    cur_doc_id;
	float       max_exp_score;
	uint32_t    n_occurs;
	hit_occur_t occurs[MAX_HIGHLIGHT_OCCURS];

	/* candidate docID, expID */
	uint64_t candidate;

	/* for pruning */
	struct math_pruner pruner;
};

struct l2_postlist_item {
	uint32_t    doc_id;
	float       part_score;
	uint32_t    n_occurs;
	hit_occur_t occurs[MAX_HIGHLIGHT_OCCURS];
};

ranked_results_t
indices_run_query(struct indices*, struct query*);
