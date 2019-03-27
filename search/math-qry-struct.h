#pragma once
#include "config.h"
#include "indices/indices.h"
#include "math-prefix-qry.h"

/* search irrelevant math query structure */
struct math_qry_struct {
	struct math_prefix_qry pq; /* except this! */
	struct subpaths subpaths;
	list subpath_set;
	int n_uniq_paths;
	int n_qry_nodes;
	uint32_t visibimap[MAX_NODE_IDS];
	void *optr;
	char *kw_str;
};

int math_qry_prepare(struct indices*, char*, struct math_qry_struct*);

void math_qry_free(struct math_qry_struct*);
