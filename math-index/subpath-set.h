#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "list/list.h"

#define MAX_QRY_MATHS 32

struct subpath_ele {
	struct list_node ln;
	uint32_t         dup_cnt;
	struct subpath  *dup[MAX_MATH_PATHS];
	uint32_t         rid[MAX_MATH_PATHS]; /*  node ID of prefix root */
	uint32_t         prefix_len;
};

struct subpath_ele_added {
	uint32_t new_uniq;
	uint32_t new_dups;
};

LIST_DECL_FREE_FUN(subpath_set_free);

struct subpath_ele_added
subpath_set_add(list*, struct subpath*, int);

void subpath_set_print(list*, FILE*);
void subpath_set_print_ele(struct subpath_ele*);

struct subpath_ele_added
lr_subpath_set_from_subpaths(struct subpaths*, list* /* set output */);

struct subpath_ele_added
prefix_subpath_set_from_subpaths(struct subpaths*, list*  /* set output */);

void delete_gener_paths(struct subpaths*);
