struct sector_tr {
	int rnode;
	int height;
	int width;
};

#include "postmerge/config.h" // for MAX_MERGE_POSTINGS
#include <stdint.h>
struct pruner_node {
	int width;
	int n; // number of sector trees
	struct sector_tr secttr[MAX_MERGE_POSTINGS]; // each sect rnode is the same here
	int postlist_id[MAX_MERGE_POSTINGS];
};

struct math_pruner {
	struct pruner_node *nodes;
	uint32_t n_nodes;
	int postlist_ref[MAX_MERGE_POSTINGS]; // reference counter
};

#include "math-index/head.h"
int math_pruner_init(struct math_pruner*, uint32_t,
                     struct subpath_ele**, uint32_t);

void math_pruner_free(struct math_pruner*);

void math_pruner_print(struct math_pruner*);

void math_pruner_dele_node(struct math_pruner*, int);
