#pragma once
struct sector_tr {
	int rnode;
	int width;
};

#include "postmerge/config.h" // for MAX_MERGE_POSTINGS
#include <stdint.h>
struct pruner_node {
	int width;
	int n; // number of sector trees
	struct sector_tr secttr[MAX_MERGE_POSTINGS]; // each sect rnode is the same
	int postlist_id[MAX_MERGE_POSTINGS];
};

struct node_set {
	int *rid; /* root of reference sector tree */
	int *ref; /* width of reference sector tree */
	int  sz;
};

#include "config.h"
#include "hashtable/u16-ht.h"
#include "bin-lp.h"
struct math_pruner {
	/* internal node related */
	struct pruner_node *nodes; // query internal nodes
	uint16_t *nodeID2idx; // nodeID to node index map
	uint32_t n_nodes; // number of internal nodes in query

	/* posting list related */
	int postlist_pivot; // advance-skip seperator
	struct node_set postlist_nodes[MAX_MERGE_POSTINGS]; // back-references
	int postlist_ref[MAX_MERGE_POSTINGS]; // reference counter
	int postlist_max[MAX_MERGE_POSTINGS]; // max sector width
	int postlist_len[MAX_MERGE_POSTINGS]; // postlist path prefix-length

	/* upperbound pre-calculations */
	float upp[MAX_LEAVES + 1];

	/* threshold records */
	float  init_threshold, prev_threshold;

	/* binary 0-1 linear-programming problem */
	struct bin_lp blp;

	/* hash tables */
	struct u16_ht accu_ht, sect_ht;
	struct u16_ht q_hit_nodes_ht;
};

#include "math-index/head.h"
int math_pruner_init(struct math_pruner*, uint32_t,
                     struct subpath_ele**, uint32_t);

void math_pruner_free(struct math_pruner*);

void math_pruner_print(struct math_pruner*);

void math_pruner_print_postlist(struct math_pruner*, int);

void math_pruner_dele_node(struct math_pruner*, int);

void math_pruner_dele_node_safe(struct math_pruner*, int, int*, int);

/* update data structure after a node is deleted */
void math_pruner_update(struct math_pruner*);

void math_pruner_init_threshold(struct math_pruner*, uint32_t);

void math_pruner_precalc_upperbound(struct math_pruner*, uint32_t);
