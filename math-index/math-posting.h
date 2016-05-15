#pragma once
#include <stdbool.h>

typedef void* math_posting_t;

#pragma pack(push, 1)
struct math_posting_item {
	/*
	 * two IDs starting from offset zero is good for
	 * later transforming them to uint64_t.
	 */
	doc_id_t  doc_id;
	exp_id_t  exp_id;

	uint64_t  pathinfo_pos;
};

struct math_pathinfo {
	uint32_t    path_id;
	symbol_id_t lf_symb;
	symbol_id_t fr_hash;
};

struct math_pathinfo_pack {
	pathinfo_num_t        n_paths;
	uint32_t              n_lr_paths;
	struct math_pathinfo  pathinfo[];
};
#pragma pack(pop)

struct subpath_ele;
math_posting_t math_posting_new_reader(struct subpath_ele*, const char*);

void math_posting_free_reader(math_posting_t);

struct subpath_ele *math_posting_get_ele(math_posting_t);
const char *math_posting_get_pathstr(math_posting_t);

bool math_posting_start(math_posting_t);
bool math_posting_jump(math_posting_t, uint64_t);
bool math_posting_next(math_posting_t);
void math_posting_finish(math_posting_t);

struct math_posting_item*
math_posting_current(math_posting_t);

struct math_pathinfo_pack*
math_posting_pathinfo(math_posting_t, uint64_t);

/* print math posting path and its subpath set duplicate elements */
void math_posting_print_info(math_posting_t);
