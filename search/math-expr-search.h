#include <stdbool.h>
#include "math-index/math-index.h"
#include "math-index/subpath-set.h"

#include "postmerge.h"
#include "mnc-score.h"
#include "math-prefix-qry.h"

struct math_extra_score_arg {
	uint32_t n_qry_lr_paths;
	uint32_t dir_merge_level;
	uint32_t n_dir_visits;
	bool     stop_dir_search;
	void    *expr_srch_arg;
	struct math_prefix_qry pq;
};

#pragma pack(push, 1)
struct math_expr_score_res {
	doc_id_t  doc_id;
	exp_id_t  exp_id;
	uint32_t  score;
};
#pragma pack(pop)

/* perform math expression search. Upon math posting list merge,
 * call the callback function specified in the argument. */
int64_t math_expr_search(math_index_t, char*, enum dir_merge_type,
                         post_merge_callbk, void*);

/* call this function in posting merge callback to score merged item
 * similarity compared with math query. */
struct math_expr_score_res
math_expr_score_on_merge(struct postmerge*, uint32_t, uint32_t);
