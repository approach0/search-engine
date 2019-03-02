#pragma once
#include "config.h"

#define MAX_MATH_POSTLIST_PATHINFO_LEN  64

struct math_postlist_item {
	uint32_t  exp_id; /* lower address, less significant */
	uint32_t  doc_id; /* higher address, more significant */

	uint16_t  n_lr_paths;
	uint16_t  n_paths;

	uint16_t  subr_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint16_t  op_hash[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint16_t  lf_symb[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint16_t  leaf_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
};

struct math_postlist_gener_item {
	uint32_t  exp_id; /* lower address, less significant */
	uint32_t  doc_id; /* higher address, more significant */

	uint16_t  n_lr_paths;
	uint16_t  n_paths;

	uint16_t  subr_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint16_t  op_hash[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint16_t  tr_hash[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint16_t  wild_id[MAX_MATH_POSTLIST_PATHINFO_LEN];

	uint64_t  wild_leaves[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* additional */
};

struct postlist;

struct postlist *math_postlist_create_plain();

struct postlist *math_postlist_create_compressed();
