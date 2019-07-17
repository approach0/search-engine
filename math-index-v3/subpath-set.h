#pragma once
#include <stdint.h>

struct sector_tree {
	uint32_t rootID;
	uint32_t width;
	uint16_t ophash;
};

struct subpath_ele {
	struct list_node   ln;
	uint32_t           dup_cnt;
	struct subpath    *dup[MAX_MATH_PATHS];
	uint32_t           rid[MAX_MATH_PATHS]; /*  node ID of prefix root */
	uint32_t           prefix_len;

	uint32_t           n_sects;
	struct sector_tree secttr[MAX_MATH_PATHS];

	uint32_t           n_splits[MAX_MATH_PATHS];
	uint16_t           symbol[MAX_MATH_PATHS][MAX_MATH_PATHS];
	uint16_t           splt_w[MAX_MATH_PATHS][MAX_MATH_PATHS];
	uint64_t           leaves[MAX_MATH_PATHS][MAX_MATH_PATHS];
};
