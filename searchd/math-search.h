#include "math-index/math-index.h"
#include "math-index/subpath-set.h"

#include "postmerge.h"
#include "mnc-score.h"

struct math_extra_score_arg {
	uint32_t n_qry_lr_paths;
	uint32_t dir_merge_level;
};

struct math_score_res {
	doc_id_t  doc_id;
	exp_id_t  exp_id;
	uint32_t  score;
};

/* perform math expression search. Upon math posting list merge,
 * call the callback function specified in the argument. */
int math_search_posting_merge(math_index_t, char*, enum dir_merge_type,
                              post_merge_callbk, void*);

/* call this function in posting merge callback to score merged item
 * similarity compared with math query. */
struct math_score_res
math_score_on_merge(struct postmerge_arg*, uint32_t, uint32_t);
