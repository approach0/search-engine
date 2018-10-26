#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math-pruning.h"

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

	/* calculate the width for each node,
	 * and reference counter for each posting list. */
	memset(pruner->postlist_ref, 0, MAX_MERGE_POSTINGS * sizeof(int));
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *node = pruner->nodes + i;
		int sum = 0;
		for (int j = 0; j < node->n; j++) {
			int pid = node->postlist_id[j];
			pruner->postlist_ref[pid] += 1;

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

	return 0;
}

void math_pruner_print(struct math_pruner *pruner)
{
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *node = pruner->nodes + i;
		printf("node#%d / %d: \n", node->secttr[0].rnode, node->width);
		for (int j = 0; j < node->n; j++) {
			int pid = node->postlist_id[j];
			printf("\t sector tree: (#%d, %d) ---> posting_list[%d], ref: %d\n",
				node->secttr[j].rnode, node->secttr[j].width,
				pid, pruner->postlist_ref[pid]);
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
}
