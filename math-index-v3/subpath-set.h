#pragma once
#include <stdint.h>
#include "linkli/list.h"

#include "tex-parser/tex-parser.h" /* for MAX_SUBPATH_ID */
#define MAX_MATH_PATHS MAX_SUBPATH_ID

struct sector_tree {
	uint32_t rootID;
	uint32_t width;
	uint16_t ophash;
};

struct subpath_ele {
	struct li_node     ln;
	uint32_t           dup_cnt;
	struct subpath    *dup[MAX_MATH_PATHS];
	uint32_t           rid[MAX_MATH_PATHS];  /* root node ID */
	uint32_t           rtok[MAX_MATH_PATHS]; /* root token */
	uint32_t           prefix_len;

	uint32_t           n_sects;
	struct sector_tree secttr[MAX_MATH_PATHS];

	uint32_t           n_splits[MAX_MATH_PATHS];
	uint16_t           symbol[MAX_MATH_PATHS][MAX_MATH_PATHS];
	uint16_t           splt_w[MAX_MATH_PATHS][MAX_MATH_PATHS];
	uint64_t           leaves[MAX_MATH_PATHS][MAX_MATH_PATHS];
};

enum subpath_set_opt {
	SUBPATH_SET_DOC,
	SUBPATH_SET_QUERY
};

linkli_t subpath_set(struct subpaths, enum subpath_set_opt);
