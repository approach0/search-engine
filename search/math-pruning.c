#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "math-expr-sim.h"
#include "math-pruning.h"

void math_pruner_update(struct math_pruner *pruner)
{
	/* first, update nodeID2idx */
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

		pruner->postlist_len[i] = ele->prefix_len;
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

	/* set pivot (after pivot should be skip-only posting lists) */
	pruner->postlist_pivot = n_eles - 1;

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
				if (pid == i)
					save_rid[cnt++] = node->secttr[k].rnode;
			}
		}
		pruner->postlist_nodes[i].sz = cnt;
		pruner->postlist_nodes[i].rid = malloc(cnt * sizeof(int));
		for (int j = 0; j < cnt; j++)
			pruner->postlist_nodes[i].rid[j] = save_rid[j];
	}

	/* allocate nodeID2idx table */
	pruner->nodeID2idx = calloc(1 + n_nodes, sizeof(uint16_t));

	/* nodeID2idx is allocated, setup dynamical data structures */
	math_pruner_update(pruner);

	/* hashtable initialization */
	pruner->accu_ht = u16_ht_new(0);
	pruner->sect_ht = u16_ht_new(0);
	pruner->q_hit_nodes_ht = u16_ht_new(0);

	/* pre-calc factors for score pruning */
	pruner->prev_threshold = -1.f;
	pruner->init_threshold = 0.f;

	return 0;
}

void math_pruner_free(struct math_pruner* pruner)
{
	/* free query node records */
	free(pruner->nodes);

	/* free back-references to nodes */
	for (int i = 0; i < MAX_MERGE_POSTINGS; i++)
		if (pruner->postlist_nodes[i].rid)
			free(pruner->postlist_nodes[i].rid);

	/* free nodeID2idx table */
	free(pruner->nodeID2idx);

	/* free hashtables */
	u16_ht_free(&pruner->accu_ht);
	u16_ht_free(&pruner->sect_ht);
	u16_ht_free(&pruner->q_hit_nodes_ht);
}

void math_pruner_init_threshold(struct math_pruner* pruner, uint32_t _qw)
{
	float qw = (float)_qw;
	float inv_qw = 1.f / qw;
	float min_mt = floorf(MATH_PRUNING_MIN_THRESHOLD_FACTOR * qw);
	float max_dw = (float)MAX_LEAVES;
	pruner->init_threshold =
		math_expr_score_lowerbound(min_mt, max_dw, inv_qw);
}

void math_pruner_precalc_upperbound(struct math_pruner* pruner, uint32_t _qw)
{
	float qw = (float)_qw;
	float inv_qw = 1.f / qw;
	for (int i = 1; i <= MAX_LEAVES; i++) {
		for (int j = 1; j <= MAX_LEAVES; j++) {
			pruner->upp[i][j] =
				math_expr_score_upperbound(i, j, inv_qw);
		}
	}
}

void math_pruner_print_postlist(struct math_pruner *pruner, int pid)
{
	printf("po#%d/%d", pid, pruner->postlist_max[pid]);
	printf(" (ref: %d) -> ", pruner->postlist_ref[pid]);
	for (int k = 0; k < pruner->postlist_nodes[pid].sz; k++) {
		int rid = pruner->postlist_nodes[pid].rid[k];
		int idx = pruner->nodeID2idx[rid];
		struct pruner_node *qnode = pruner->nodes + idx;
		printf("node#%d/%d ", rid, qnode->width);
	}
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
			printf("max: %d, len: %d, ref: %d [", pruner->postlist_max[pid],
				pruner->postlist_len[pid], pruner->postlist_ref[pid]);
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

void
math_pruner_dele_node_safe(struct math_pruner *pruner, int i, int *x, int n)
{
	/* update x */
	for (int j = 0; j < n; j++) if (x[j] > i) x[j] --;
	math_pruner_dele_node(pruner, i);
}
