#pragma once

#include "postmerge/postmerger.h"
#include "math-qry-struct.h"
#include "math-pruning.h"
#include "search.h"

enum math_postlist_medium {
	MATH_POSTLIST_ONDISK,
	MATH_POSTLIST_INMEMO,
	MATH_POSTLIST_EMPTYMEM
};

struct math_l2_postlist {
	struct postmerger pm; /* posting lists */
	postmerger_iter_t iter;

	enum math_postlist_medium medium[MAX_MERGE_POSTINGS];
	enum math_posting_type path_type[MAX_MERGE_POSTINGS];
	struct subpath_ele *ele[MAX_MERGE_POSTINGS];

	struct math_qry_struct *mqs;
	struct indices *indices /* for debug */;
	ranked_results_t *rk_res;

	/* current doc-level item recorder */
	uint32_t    cur_doc_id, future_doc_id;
	float       max_exp_score;
	uint32_t    n_occurs;
	hit_occur_t occurs[MAX_MATH_OCCURS];

	/* candidate docID, expID */
	uint64_t candidate;

	/* for pruning */
	struct math_pruner pruner;
};

struct math_l2_postlist_item {
	uint32_t     doc_id;
	float        part_score;
	uint32_t     n_occurs;
	hit_occur_t  occurs[MAX_MATH_OCCURS];
};

void math_l2_postlist_print(struct math_l2_postlist*);

struct math_l2_postlist
math_l2_postlist(struct indices*, struct math_qry_struct*, ranked_results_t*);

struct postmerger_postlist
postmerger_math_l2_postlist(struct math_l2_postlist*);

int math_search_pause_toggle(); /* for debug */
