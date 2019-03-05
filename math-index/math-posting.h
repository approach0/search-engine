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

struct math_posting_compound_item_v1 {
	exp_id_t              exp_id;
	doc_id_t              doc_id;
	uint32_t              n_paths;
	uint32_t              n_lr_paths;
	struct math_pathinfo  pathinfo[MAX_MATH_PATHS];
};

/* v2 */
struct math_posting_item_v2 {
	uint32_t        exp_id     :20; /* lower address, less significant */
	uint32_t        n_lr_paths :6; /* number of paths in original tree */
	uint32_t        n_paths    :6; /* number of different paths followed */

	doc_id_t        doc_id; /* higher address, more significant */

	uint32_t  pathinfo_pos; /* pointing to path info */
};

struct math_pathinfo_v2 {
	uint16_t    leaf_id;
	uint16_t    subr_id;
	symbol_id_t lf_symb;
	symbol_id_t op_hash;
};

struct math_pathinfo_gener_v2 {
	uint16_t    wild_id;
	uint16_t    subr_id;
	symbol_id_t tr_hash;
	symbol_id_t op_hash;
	uint64_t    wild_leaves;
};
#pragma pack(pop)

struct subpath_ele;

enum math_posting_type {
	TYPE_PREFIX,
	TYPE_GENER
};

math_posting_t
math_posting_new_reader(const char*);

void math_posting_free_reader(math_posting_t);

const char *math_posting_get_pathstr(math_posting_t);

int math_posting_signature(math_posting_t);
enum math_posting_type math_posting_type(math_posting_t);

/* some writing functions to write mock posting */
int write_posting_item_v2(const char*, struct math_posting_item_v2*);
uint32_t get_pathinfo_len(const char*);

bool math_posting_start(math_posting_t);
bool math_posting_jump(math_posting_t, uint64_t);
bool math_posting_next(math_posting_t);
void math_posting_finish(math_posting_t);

uint64_t math_posting_cur_id_v1(math_posting_t);
uint64_t math_posting_cur_id_v2(math_posting_t);
uint64_t math_posting_cur_id_v2_2(math_posting_t);

void *math_posting_cur_item_v1(math_posting_t);

int math_posting_exits(const char*);

/* 2nd generation iterator */
typedef void* math_posting_iter_t;
math_posting_iter_t math_posting_iterator(const char*);
math_posting_iter_t math_posting_copy(math_posting_iter_t);
int      math_posting_empty(const char*);
uint64_t math_posting_iter_cur(math_posting_iter_t);
int      math_posting_iter_next(math_posting_iter_t);
int      math_posting_iter_jump(math_posting_iter_t, uint64_t);
void     math_posting_iter_free(math_posting_iter_t);

size_t math_posting_read(math_posting_t, void*);
size_t math_posting_read_gener(math_posting_t, void*);
