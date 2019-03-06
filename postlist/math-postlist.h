#pragma once
#include "config.h"

#define MAX_MATH_POSTLIST_PATHINFO_LEN  64

struct math_postlist_item {
	/* 0 */ uint32_t  exp_id; /* lower address, less significant */
	/* 1 */ uint32_t  doc_id; /* higher address, more significant */
	/* 2 */ uint32_t  n_lr_paths;
	/* 3 */ uint32_t  n_paths;
	/* 4 */ uint32_t  leaf_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	/* 5 */ uint32_t  subr_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	/* 6 */ uint32_t  lf_symb[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* plain codec */
	/* 7 */ uint32_t  op_hash[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* plain codec */
};

struct math_postlist_gener_item {
	/* 0 */ uint32_t  exp_id; /* lower address, less significant */
	/* 1 */ uint32_t  doc_id; /* higher address, more significant */
	/* 2 */ uint32_t  n_lr_paths;
	/* 3 */ uint32_t  n_paths;
	/* 4 */ uint32_t  wild_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	/* 5 */ uint32_t  subr_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	/* 6 */ uint32_t  tr_hash[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* plain codec */
	/* 7 */ uint32_t  op_hash[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* plain codec */
	/* 8 */ uint64_t  wild_leaves[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* additional */
};

struct postlist;

struct postlist *math_postlist_create_plain();
struct postlist *math_postlist_create_compressed();

struct postlist *math_postlist_create_gener_plain();
struct postlist *math_postlist_create_gener_compressed();

typedef char *(*trans_fun_t)(uint32_t);
void math_postlist_print_item(struct math_postlist_gener_item*, int, trans_fun_t);
