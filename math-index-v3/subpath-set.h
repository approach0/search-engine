#pragma once
#include <stdint.h>
#include "linkli/list.h"
#include "config.h"

struct sector_tree {
	uint32_t rootID;
	uint32_t width;
	uint16_t ophash;
};

struct subpath_ele {
	/* unique prefix tokens */
	struct li_node     ln;
	uint32_t           dup_cnt;
	struct subpath    *dup[MAX_MATH_PATHS];
	uint32_t           rid[MAX_MATH_PATHS];  /* root node ID */
	uint32_t           prefix_len;

	/* sector trees */
	uint32_t           n_sects;
	struct sector_tree secttr[MAX_MATH_PATHS];

	/* symbol splits for each sector tree */
	uint32_t           n_splits[MAX_MATH_PATHS];
	uint16_t           symbol[MAX_MATH_PATHS][MAX_MATH_PATHS];
	uint16_t           splt_w[MAX_MATH_PATHS][MAX_MATH_PATHS];
	uint64_t           leaves[MAX_MATH_PATHS][MAX_MATH_PATHS];
};

enum subpath_set_opt {
	SUBPATH_SET_DOC,
	SUBPATH_SET_QUERY
};

linkli_t subpath_set(struct lr_paths, enum subpath_set_opt);

void print_subpath_set(linkli_t);
