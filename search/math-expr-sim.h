#pragma once

#include <stdbool.h>
#include "postmerge/postmerge.h"
#include "indices/indices.h"
#include "mnc-score.h"
#include "math-prefix-qry.h"
#include "config.h"

struct math_extra_score_arg;

struct math_expr_score_res {
	doc_id_t  doc_id;
	exp_id_t  exp_id;
	float     score;
};

struct math_expr_sim_factors {
	mnc_score_t mnc_score;
	uint32_t srch_depth;
	uint32_t qry_lr_paths, doc_lr_paths;
	struct pq_align_res *align_res;
	uint32_t k, joint_nodes;
	int lcs;
	uint32_t qry_nodes;
};

void math_expr_set_score(struct math_expr_sim_factors*,
                         struct math_expr_score_res*);

#include "search.h"
void math_l2_postlist_print_cur(struct math_l2_postlist*);

struct math_expr_score_res
math_l2_postlist_cur_score(struct math_l2_postlist*);

mnc_score_t math_l2_postlist_cur_symbol_sim(struct math_l2_postlist*, struct pq_align_res*);

struct pq_align_res math_l2_postlist_coarse_score(struct math_l2_postlist*, uint32_t);

struct pq_align_res math_l2_postlist_widest_match(struct math_l2_postlist*, float);

struct math_expr_score_res
math_l2_postlist_precise_score(struct math_l2_postlist*, struct pq_align_res*, uint32_t);

float math_expr_score_upperbound(int, int);
float math_expr_score_lowerbound(int, int);

void math_expr_sim_factors_print(struct math_expr_sim_factors*);
