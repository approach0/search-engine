#include <stdbool.h>
#include "postmerge.h"
#include "mnc-score.h"

#pragma pack(push, 1)
struct math_expr_score_res {
	doc_id_t  doc_id;
	exp_id_t  exp_id;
	uint32_t  score;
};
#pragma pack(pop)

uint32_t
math_expr_sim(mnc_score_t, uint32_t, uint32_t, uint32_t, uint32_t, int);

struct math_expr_score_res
math_expr_score_on_merge(struct postmerge*, uint32_t, uint32_t);

struct math_expr_score_res
math_expr_prefix_score_on_merge(uint64_t, struct postmerge*, uint32_t, struct math_prefix_qry*, uint32_t);
