#pragma once
#include "config.h"

#define MAX_MATH_POSTLIST_PATHINFO_LEN  64

struct math_postlist_item {
	/* 0 */ uint32_t  exp_id; /* lower address, less significant */
	/* 1 */ uint32_t  doc_id; /* higher address, more significant */
	/* 2 */ uint32_t  n_lr_paths;
	/* 3 */ uint32_t  n_paths;
	/* 4 */ uint32_t  subr_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	/* 5 */ uint32_t  op_hash[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* actually 16 bits */
	/* 6 */ uint32_t  lf_symb[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* actually 16 bits */
	/* 7 */ uint32_t  leaf_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
}; /* 1024 bytes */

#define MAX_EXP_SYMBOL_SPLITS 4
struct math_invlist_item {
	uint32_t doc_id;
	uint32_t sec_id; /* sector tree ID: formulaID (20 bits) appended by nodeID (12 bits) */
	uint32_t offset; /* offset of corresponding position in another file */
	uint16_t width; /* sector tree width appended by original formula width (n_lr_paths) */

	/* --- in two separate files --- */
	uint16_t splits; /* how many different symbol values, appended by operator signature */
	/* 16 bytes */

	uint16_t symbol[MAX_EXP_SYMBOL_SPLITS]; /* leaf symbol */
	uint16_t splt_w[MAX_EXP_SYMBOL_SPLITS]; /* split width */
	/* 32 bytes */
	//uint64_t leaves[MAX_EXP_SYMBOL_SPLITS]; /* bit mask signature (for wildcards) */
};

struct math_postlist_gener_item {
	/* 0 */ uint32_t  exp_id; /* lower address, less significant */
	/* 1 */ uint32_t  doc_id; /* higher address, more significant */
	/* 2 */ uint32_t  n_lr_paths;
	/* 3 */ uint32_t  n_paths;
	/* 4 */ uint32_t  subr_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	/* 5 */ uint32_t  op_hash[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* actually 16 bits */
	/* 6 */ uint32_t  tr_hash[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* actually 16 bits */
	/* 7 */ uint64_t  wild_leaves[MAX_MATH_POSTLIST_PATHINFO_LEN]; /* difference */
};

struct postlist;

struct postlist *math_postlist_create_plain();
struct postlist *math_postlist_create_compressed();

struct postlist *math_postlist_create_gener_plain();
struct postlist *math_postlist_create_gener_compressed();

typedef char *(*trans_fun_t)(uint32_t);
void math_postlist_print_item(struct math_postlist_gener_item*, int, trans_fun_t);
