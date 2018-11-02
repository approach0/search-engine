struct sector_tr {
	int rnode;
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

struct node_set {
	int *rid;
	int  sz;
};

struct math_pruner {
	struct pruner_node *nodes;
	uint16_t           *nodeID2idx; // nodeID to node index map
	uint32_t            n_nodes;
	int             postlist_pivot; // advance-skip seperator
	struct node_set postlist_nodes[MAX_MERGE_POSTINGS]; // back-reference to nodes
	int postlist_ref[MAX_MERGE_POSTINGS]; // reference counter
	int postlist_max[MAX_MERGE_POSTINGS]; // max sector width
};

#include "math-index/head.h"
int math_pruner_init(struct math_pruner*, uint32_t,
                     struct subpath_ele**, uint32_t);

void math_pruner_free(struct math_pruner*);

void math_pruner_print(struct math_pruner*);

void math_pruner_dele_node(struct math_pruner*, int);

/* update data structure after a node is deleted */
void math_pruner_update(struct math_pruner*);
