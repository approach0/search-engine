#pragma once

#include <stdbool.h>
#include "math-index/math-index.h"
#include "math-index/subpath-set.h"
#include "postmerge/postmerge.h"
#include "indices/indices.h"

#include "math-prefix-qry.h"
#include "math-expr-sim.h"

enum math_postlist_type {
	MATH_POSTLIST_TYPE_EMPTY,
	MATH_POSTLIST_TYPE_MEMORY,
	MATH_POSTLIST_TYPE_DISK_V1,
	MATH_POSTLIST_TYPE_DISK_V2
};

struct math_extra_posting_arg {
	char                   *full_path;
	char                   *base_path;
	struct subpath_ele     *ele;
	enum math_postlist_type type;
};

struct math_extra_score_arg {
	uint32_t n_qry_lr_paths;
	uint32_t dir_merge_level;
	uint32_t n_dir_visits;
	bool     stop_dir_search;
	void    *expr_srch_arg;
	struct math_prefix_qry pq;
};

enum math_expr_search_policy {
	MATH_SRCH_EXACT_STRUCT,
	MATH_SRCH_FUZZY_STRUCT,
	MATH_SRCH_SUBEXPRESSION
};

int64_t math_postmerge(struct indices *indices, char *tex,
                       enum math_expr_search_policy search_policy,
                       post_merge_callbk fun, void *args);

#include "rank.h"
ranked_results_t
math_expr_search(struct indices*, char*, enum math_expr_search_policy);
