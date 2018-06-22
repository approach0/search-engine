#include <stdbool.h>
#include "math-index/math-index.h"
#include "math-index/subpath-set.h"
#include "postmerge/postmerge.h"

#include "math-prefix-qry.h"
#include "math-expr-sim.h"

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

/* perform math expression search. Upon math posting list merge,
 * call the callback function specified in the argument. */
int64_t math_expr_search(math_index_t, char*, enum math_expr_search_policy,
                         post_merge_callbk, void*);
