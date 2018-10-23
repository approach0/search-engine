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

		for (int j = 0; j < ele->dup_cnt + 1; j++) {
			uint32_t r = ele->rid[j];
			uint32_t h = ele->dup[j]->n_nodes;
			struct pruner_node *node = pruner->nodes + (r - 1);

			int k;
			for (k = 0; k < node->n; k++)
				if (node->secttr[k].height == h)
					break;
		
			if (k == node->n) node->n += 1;
			node->secttr[k].rnode = r;
			node->secttr[k].height = h;
			node->secttr[k].width += 1;
			node->postlist_id[k] = i;
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
			printf("\t sector tree: (%d, %d) ---> posting_list[%d], ref: %d\n",
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
