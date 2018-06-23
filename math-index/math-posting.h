#pragma once
#include <stdbool.h>

typedef void* math_posting_t;

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "Not little-endian machine."
#endif

#pragma pack(push, 1)
struct math_posting_item {
	/*
	 * two IDs will be casted to uint64_t
	 */
	exp_id_t  exp_id; /* lower address, less significant */
	doc_id_t  doc_id; /* higher address, more significant */

	uint32_t  pathinfo_pos;
};

struct math_pathinfo {
	uint32_t    path_id;
	symbol_id_t lf_symb;
	symbol_id_t fr_hash;
};

struct math_pathinfo_pack {
	pathinfo_num_t        n_paths; /* number of different paths followed */
	uint32_t              n_lr_paths; /* number of paths in original tree */
	struct math_pathinfo  pathinfo[];
};

struct math_posting_item_v2 {
	uint32_t        exp_id     :20; /* lower address, less significant */
	uint32_t        n_lr_paths :6; /* number of paths in original tree */
	uint32_t        n_paths    :6; /* number of different paths followed */

	doc_id_t        doc_id; /* higher address, more significant */

	uint32_t  pathinfo_pos; /* pointing to path info */
};

struct math_pathinfo_v2 {
	uint32_t    leaf_id;
	uint32_t    subr_id;
	symbol_id_t lf_symb;
};
#pragma pack(pop)

struct math_posting_compound_item_v1 {
	exp_id_t              exp_id;
	doc_id_t              doc_id;
	uint32_t              n_paths;
	uint32_t              n_lr_paths;
	struct math_pathinfo  pathinfo[MAX_MATH_PATHS];
};

struct math_posting_compound_item_v2 {
	exp_id_t                 exp_id;
	doc_id_t                 doc_id;
	uint32_t                 n_paths;
	uint32_t                 n_lr_paths;
	struct math_pathinfo_v2  pathinfo[MAX_MATH_PATHS];
};

struct subpath_ele;

math_posting_t
math_posting_new_reader(const char*);

void math_posting_free_reader(math_posting_t);

const char *math_posting_get_pathstr(math_posting_t);
int math_posting_signature(math_posting_t);

bool math_posting_start(math_posting_t);
bool math_posting_jump(math_posting_t, uint64_t);
bool math_posting_next(math_posting_t);
void math_posting_finish(math_posting_t);

void *math_posting_cur_item_v1(math_posting_t);
void *math_posting_cur_item_v2(math_posting_t);

int math_posting_exits(const char*);
