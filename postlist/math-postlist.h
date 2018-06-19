#include "config.h"

#define MAX_MATH_POSTLIST_PATHINFO_LEN  64

struct math_postlist_item {
	uint32_t  exp_id; /* lower address, less significant */
	uint32_t  doc_id; /* higher address, more significant */

	uint32_t  n_lr_paths;
	uint32_t  n_paths;

	uint32_t  leaf_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint32_t  subr_id[MAX_MATH_POSTLIST_PATHINFO_LEN];
	uint32_t  lf_symb[MAX_MATH_POSTLIST_PATHINFO_LEN];
};

struct postlist;

struct postlist *math_postlist_create_plain();

struct postlist *math_postlist_create_compressed();
