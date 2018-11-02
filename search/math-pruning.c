#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math-pruning.h"

void math_pruner_update(struct math_pruner *pruner)
{
	/* first, update nodeID2idx */
	pruner->nodeID2idx = malloc(pruner->n_nodes * sizeof(uint16_t));
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *node = pruner->nodes + i;
		int node_id = node->secttr[0].rnode;
		pruner->nodeID2idx[node_id] = i;
	}

	/* second, update postlist_ref and postlist_max */
	memset(pruner->postlist_ref, 0, MAX_MERGE_POSTINGS * sizeof(int));
	memset(pruner->postlist_max, 0, MAX_MERGE_POSTINGS * sizeof(int));
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *node = pruner->nodes + i;
		// int rid = node->secttr[0].rnode;

		for (int j = 0; j < node->n; j++) {
			/* update postlist_ref */
			int pid = node->postlist_id[j];
			pruner->postlist_ref[pid] += 1;
			/* update postlist_max */
			int width = node->secttr[j].width;
			if (pruner->postlist_max[pid] < width)
				pruner->postlist_max[pid] = width;
		}
	}
}

int
math_pruner_init(struct math_pruner* pruner, uint32_t n_nodes,
                 struct subpath_ele** eles, uint32_t n_eles)
{
	/* place query nodes into pruner */
	pruner->nodes = calloc(n_nodes, sizeof(struct pruner_node));

	for (int i = 0; i < n_eles; i++) {
		struct subpath_ele *ele = eles[i];

		int dirty_r[n_nodes];
		memset(dirty_r, 0, sizeof(dirty_r));
		for (int j = 0; j < ele->dup_cnt + 1; j++) {
			uint32_t r = ele->rid[j];
			struct pruner_node *node = pruner->nodes + (r - 1);
			if (!dirty_r[r - 1]) {
				dirty_r[r - 1] = 1;
				/* allocate new sector tree */
				node->n += 1;
				node->secttr[node->n - 1].rnode = r;
				node->postlist_id[node->n - 1] = i;
			}

			/* widen sector tree */
			node->secttr[node->n - 1].width += 1;
		}
	}
	
	/* squash query nodes into consecutive index */
	pruner->n_nodes = 0;
	for (int i = 0; i < n_nodes; i++) {
		struct pruner_node *ni = pruner->nodes + i;

		if (ni->secttr[0].rnode > 0) {
			struct pruner_node *nj = pruner->nodes + pruner->n_nodes;
			struct pruner_node tmp;
			tmp = *nj;
			*nj = *ni;
			*ni = tmp;
			pruner->n_nodes ++;
		}
	}

	/* calculate the width for each node. */
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *node = pruner->nodes + i;
		int sum = 0;
		for (int j = 0; j < node->n; j++) {
			sum += node->secttr[j].width;
		}
		node->width = sum;
	}

	/* bubble sort node by their upperbounds */
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *ni = pruner->nodes + i;

		for (int j = i + 1; j < pruner->n_nodes; j++) {
			struct pruner_node *nj = pruner->nodes + j;

			if (ni->width < nj->width) {
				struct pruner_node tmp;
				tmp = *nj;
				*nj = *ni;
				*ni = tmp;
			}
		}
	}

	/* set pivot */
	pruner->postlist_pivot = n_eles;

	/* setup postlist_nodes */
	for (int i = 0; i < MAX_MERGE_POSTINGS; i++)
		pruner->postlist_nodes[i].rid = NULL;

	for (int i = 0; i < n_eles; i++) {
		int save_rid[pruner->n_nodes];
		int cnt = 0;
		for (int j = 0; j < pruner->n_nodes; j++) {
			struct pruner_node *node = pruner->nodes + j;
			for (int k = 0; k < node->n; k++) {
				int pid = node->postlist_id[k];
				if (pid == i) {
					save_rid[cnt++] = node->secttr[k].rnode;
				}
			}
		}
		pruner->postlist_nodes[i].sz = cnt;
		pruner->postlist_nodes[i].rid = malloc(cnt * sizeof(int));
		for (int j = 0; j < cnt; j++) {
			pruner->postlist_nodes[i].rid[j] = save_rid[j];
		}
	}

	/* set other v2 pruning data structures */
	math_pruner_update(pruner);

	return 0;
}

void math_pruner_print(struct math_pruner *pruner)
{
	printf("math pruner: (pivot = %d)\n", pruner->postlist_pivot);
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *node = pruner->nodes + i;
		int rid = node->secttr[0].rnode;
		printf("[%d] node#%d / %d: \n", pruner->nodeID2idx[rid], rid,
		       node->width);
		for (int j = 0; j < node->n; j++) {
			int pid = node->postlist_id[j];
			printf("\t sector tree: (#%d, %d) ---> posting_list[%d], ",
				node->secttr[j].rnode, node->secttr[j].width, pid);
			printf("ref: %d, max: %d [", pruner->postlist_ref[pid],
			                             pruner->postlist_max[pid]);
			for (int k = 0; k < pruner->postlist_nodes[pid].sz; k++) {
				printf("node#%d ", pruner->postlist_nodes[pid].rid[k]);
			}
			printf("]\n");
		}
	}
}

void math_pruner_dele_node(struct math_pruner *pruner, int i)
{
	pruner->n_nodes -= 1;
	for (int j = i; j < pruner->n_nodes; j++)
		pruner->nodes[j] = pruner->nodes[j + 1];
}

void math_pruner_free(struct math_pruner* pruner)
{
	if (pruner->nodes)
		free(pruner->nodes);

	for (int i = 0; i < MAX_MERGE_POSTINGS; i++)
		if (pruner->postlist_nodes[i].rid)
			free(pruner->postlist_nodes[i].rid);
	
	if (pruner->nodeID2idx)
		free(pruner->nodeID2idx);
}
