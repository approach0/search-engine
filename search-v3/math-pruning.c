#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "common/common.h"
#include "tex-parser/head.h"
#include "math-qry.h"
#include "math-pruning.h"

static int find_splt_idx(struct subpath_ele *ele, uint32_t root)
{
	for (int i = 0; i < ele->n_sects; i++)
		if (ele->secttr[i].rootID == root)
			return i;

	assert(0);
	return -1;
}

static void init_qnodes(struct math_pruner *pruner, struct math_qry *mq)
{
	/*
	 * store query nodes information
	 */
	pruner->qnodes = calloc(mq->n_qnodes, sizeof(struct math_pruner_qnode));

	for (int i = 0; i < mq->merge_set.n; i++) {
		struct subpath_ele *ele = mq->ele[i];
		if (ele == NULL) continue;

		/* a new element now, will create a new sector tree in each qnode */
		int dirty_r[mq->n_qnodes];
		memset(dirty_r, 0, sizeof(dirty_r));

		/* for each path of the same token sequence */
		for (int j = 0; j < ele->dup_cnt + 1; j++) {
			uint32_t r = ele->rid[j];
			struct math_pruner_qnode *qnode = pruner->qnodes + (r - 1);

			if (0 == dirty_r[r - 1]) {
				dirty_r[r - 1] = 1;
				/* create a new sector tree if this qnode has not created */
				qnode->n += 1;

				/* initialize sector tree properties */
				qnode->root = r;
				qnode->invlist_id[qnode->n - 1] = i;
				qnode->ele_splt_idx[qnode->n - 1] = find_splt_idx(ele, r);
			}

			/* accumulate sector tree width */
			qnode->secttr_w[qnode->n - 1] += 1;
		}
	}

	/*
	 * Squash query nodes into consecutive index
	 */
	pruner->n_qnodes = 0;
	for (int i = 0; i < mq->n_qnodes; i++) {
		struct math_pruner_qnode tmp, *nj, *ni = pruner->qnodes + i;
		if (ni->root > 0) {
			nj = pruner->qnodes + pruner->n_qnodes;
			tmp = *nj;
			*nj = *ni;
			*ni = tmp;
			pruner->n_qnodes ++;
		}
	}

	/*
	 * Sort sector trees in each node by their (widths * IPF)
	 * TODO: quik-sort?
	 */
	float *ipf = mq->merge_set.upp;
	for (int k = 0; k < pruner->n_qnodes; k++) {
		struct math_pruner_qnode *qn = pruner->qnodes + k;
		for (int i = 0; i < qn->n; i++) {
			for (int j = i + 1; j < qn->n; j++) {
				int i_w = qn->secttr_w[i];
				int i_iid = qn->invlist_id[i];
				float i_sum_ipf = ipf[i_iid] * (float)i_w;

				int j_w   = qn->secttr_w[j];
				int j_iid = qn->invlist_id[j];
				float j_sum_ipf = ipf[j_iid] * (float)j_w;

				if (i_sum_ipf < j_sum_ipf) {
					/* swap */
					int t1 = qn->secttr_w[i];
					int t2 = qn->invlist_id[i];
					int t3 = qn->ele_splt_idx[i];

					qn->secttr_w[i] = qn->secttr_w[j];
					qn->invlist_id[i] = qn->invlist_id[j];
					qn->ele_splt_idx[i] = qn->ele_splt_idx[j];

					qn->secttr_w[j] = t1;
					qn->invlist_id[j] = t2;
					qn->ele_splt_idx[j] = t3;
				}
			}
		}
	}

	/*
	 * Sum the sector tree widths for each node.
	 */
	for (int i = 0; i < pruner->n_qnodes; i++) {
		struct math_pruner_qnode *qn = pruner->qnodes + i;
		qn->sum_w = 0;
		qn->sum_ipf = 0;
		for (int j = 0; j < qn->n; j++) {
			int iid = qn->invlist_id[j];
			qn->sum_w += qn->secttr_w[j];
			qn->sum_ipf += ipf[iid] * (float)qn->secttr_w[j];
		}
	}
}

static void alloc_backrefs(struct math_pruner *pruner, struct math_qry *mq)
{
	memset(pruner->backrefs, 0, sizeof pruner->backrefs);

	for (int i = 0; i < mq->merge_set.n; i++) {
		int n_max_nodes = pruner->n_qnodes;
		pruner->backrefs[i].idx = malloc(n_max_nodes * sizeof(int));
		pruner->backrefs[i].ref = malloc(n_max_nodes * sizeof(int));
	}
}

static void update_backrefs(struct math_pruner *pruner)
{
	/* clear back references */
	for (int i = 0; i < pruner->mq->merge_set.n; i++) {
		pruner->backrefs[i].cnt = 0;
		pruner->backrefs[i].max = 0;
	}

	/* update back references from stored query node information */
	for (int i = 0; i < pruner->n_qnodes; i++) {
		struct math_pruner_qnode *qn = pruner->qnodes + i;
		/* for each sector tree in this node */
		for (int j = 0; j < qn->n; j++) {
			int iid = qn->invlist_id[j];
			int ref = qn->secttr_w[j];

			int c = pruner->backrefs[iid].cnt;
			pruner->backrefs[iid].idx[c] = i;
			pruner->backrefs[iid].ref[c] = ref;
			pruner->backrefs[iid].cnt = c + 1;

			if (pruner->backrefs[iid].max < ref)
				pruner->backrefs[iid].max = ref;
		}
	}
}

struct math_pruner
*math_pruner_init(struct math_qry *mq, struct math_score_factors *msf,
                  float init_threshold)
{
	struct math_pruner *pruner = malloc(sizeof *pruner);

	pruner->mq = mq;
	pruner->msf = msf;

	init_qnodes(pruner, mq);

	alloc_backrefs(pruner, mq);
	update_backrefs(pruner);

	pruner->blp = bin_lp_alloc(mq->n_qnodes, mq->merge_set.n);

	pruner->n_dropped_nodes = 0;
	pruner->n_dropped_iters = 0;

	math_pruner_update(pruner, init_threshold);
	return pruner;
}

void math_pruner_free(struct math_pruner *pruner)
{
	free(pruner->qnodes);

	for (int i = 0; i < pruner->mq->merge_set.n; i++) {
		if (pruner->backrefs[i].idx) {
			free(pruner->backrefs[i].idx);
			free(pruner->backrefs[i].ref);
		}
	}

	bin_lp_free(pruner->blp);
	free(pruner);
}

static void dele_qnode(struct math_pruner *pruner, int i)
{
	pruner->n_qnodes -= 1;
	for (int j = i; j < pruner->n_qnodes; j++)
		pruner->qnodes[j] = pruner->qnodes[j + 1];

	pruner->n_dropped_nodes += 1;
}

int math_pruner_update(struct math_pruner *pruner, float threshold)
{
	int cnt = 0;
	for (int i = 0; i < pruner->n_qnodes; i++) {
		struct math_pruner_qnode *qn = pruner->qnodes + i;
		float upp = math_score_upp(pruner->msf, qn->sum_ipf);
		if (upp <= threshold) {
			/* delete small query node */
			dele_qnode(pruner, i--);
			/* update after deletion */
			update_backrefs(pruner);

			cnt ++;
		}
	}

	pruner->threshold_ = threshold;
	return cnt;
}

float math_pruner_max_sum_ipf(struct math_pruner* pruner)
{
	float max_sum_ipf = 0;
	for (int i = 0; i < pruner->n_qnodes; i++) {
		struct math_pruner_qnode *qnode = pruner->qnodes + i;
		if (max_sum_ipf < qnode->sum_ipf)
			max_sum_ipf = qnode->sum_ipf;
	}
	return max_sum_ipf;
}

float math_pruner_init_threshold(struct math_pruner* pruner)
{
	float max_sum_ipf = math_pruner_max_sum_ipf(pruner);
	max_sum_ipf *= MATH_PRUNING_INIT_THRESHOLD_FACTOR;
	return math_score_low(pruner->msf, max_sum_ipf);
}

/*
 * Below are functions manipulating iterators
 */
void math_pruner_iters_drop(struct math_pruner *pruner, struct ms_merger *iter)
{
	for (int i = 0; i < iter->size; i++) {
		int iid = iter->map[i];
		uint64_t cur = MERGER_ITER_CALL(iter, cur, iid);

		/* drop unreferenced or terminated iterators */
		if (pruner->backrefs[iid].cnt <= 0) {
			i = ms_merger_map_remove(iter, i);
			pruner->n_dropped_iters += 1;

		} else if (cur == UINT64_MAX) {
			i = ms_merger_map_remove(iter, i);
			pruner->n_dropped_iters += 1;
		}
	}
}

static int maxref_compare(int max_i, int len_i, int max_j, int len_j)
{
	if (max_i != max_j)
		/* descending maxref value */
		return max_i < max_j;
	else
		/* ascending path length, so that longer
		 * one gets dropped or skipped first. */
		return len_i > len_j;
}

void math_pruner_iters_sort_by_maxref(struct math_pruner *pruner,
                                      struct ms_merger *iter)
{
	/* sort iterators by maxref values */
	for (int i = 0; i < iter->size; i++) {
		for (int j = i + 1; j < iter->size; j++) {
			int iid_i = iter->map[i];
			int iid_j = iter->map[j];
			int max_i = pruner->backrefs[iid_i].max;
			int max_j = pruner->backrefs[iid_j].max;
			int len_i = pruner->mq->entry[iid_i].pf;
			int len_j = pruner->mq->entry[iid_j].pf;
			if (maxref_compare(max_i, len_i, max_j, len_j)) {
				/* swap */
				iter->map[i] = iid_j;
				iter->map[j] = iid_i;
			}
		}
	}

	/* overwrite upperbound using updated maxref */
	float *ipf = pruner->mq->merge_set.upp;
	for (int i = 0; i < iter->size; i++) {
		int iid = iter->map[i];
		iter->set.upp[iid] = pruner->backrefs[iid].max * ipf[iid];
	}

	/* update accumulated sum and pivot */
	ms_merger_update_acc_upp(iter);
	ms_merger_lift_up_pivot(iter, pruner->threshold_,
	                        &math_score_upp, pruner->msf);
}

void math_pruner_iters_gbp_assign(struct math_pruner *pruner,
                                  struct ms_merger *iter, int full)
{
	struct bin_lp *blp = &pruner->blp;
	bin_lp_reset(blp);

	/* assign BP weights */
	float *ipf = pruner->mq->merge_set.upp;
	for (int i = 0; i < iter->size; i++) {
		int iid = iter->map[i];
		struct math_pruner_backref backref = pruner->backrefs[iid];
		for (int j = 0; j < backref.cnt; j++) {
			int idx = backref.idx[j];
			int ref = backref.ref[j];
			float sum_ipf = ipf[iid] * ref;
			(void)bin_lp_assign(blp, idx, iid, sum_ipf);
		}
	}

	/* assign BP objective weights */
	for (int i = 0; i < blp->n_po; i++) {
		if (full) {
			int iid = blp->po[i];
			int len = pruner->mq->entry[iid].pf;
			blp->weight[i] = (float)len;
		} else {
			blp->weight[i] = 1.f;
		}
	}

	/* greedily solve BP problem */
	int blp_pivot = bin_lp_solve(blp, pruner->threshold_,
	                             &math_score_upp, pruner->msf);
	iter->pivot = blp_pivot - 1;

#ifdef DEBUG_MATH_PRUNING
	printf("requirement set: (new pivot = %d)\n", iter->pivot);
	bin_lp_print(*blp, 0);
#endif

	/* reorder iterators */
	for (int i = 0; i < iter->size; i++)
		iter->map[i] = pruner->blp.po[i];
}

void math_pruner_print(struct math_pruner *pruner)
{
	float *ipf = pruner->mq->merge_set.upp;
	printf("[math pruner] threshold: %.2f \n", pruner->threshold_);
	for (int i = 0; i < pruner->n_qnodes; i++) {
		/* print node */
		struct math_pruner_qnode *qn = pruner->qnodes + i;
		printf("[%d] qnode#%d/%d, upp(%.2f)=%.2f: \n", i, qn->root, qn->sum_w,
			qn->sum_ipf, math_score_upp(pruner->msf, qn->sum_ipf));
		for (int j = 0; j < qn->n; j++) {
			/* print sector tree */
			int iid = qn->invlist_id[j];
			float sum_ipf = ipf[iid] * qn->secttr_w[j];
			printf("\t secttr/%d, upp(%.2f)=%.2f ", qn->secttr_w[j],
				sum_ipf, math_score_upp(pruner->msf, sum_ipf));

			/* print sector tree symbol splits */
			int l = qn->ele_splt_idx[j];
			struct subpath_ele *ele = pruner->mq->ele[iid];
			printf("{");
			for (int s = 0; s < ele->n_splits[l]; s++)
				printf("%s/%d ", trans_symbol(ele->symbol[l][s]),
				                 ele->splt_w[l][s]);
			printf("} ");

			/* print inverted list ID */
			printf("---> invlist[%d] <---[", iid);

			/* print inverted list back references */
			for (int k = 0; k < pruner->backrefs[iid].cnt; k++) {
				int idx = pruner->backrefs[iid].idx[k];
				int ref = pruner->backrefs[iid].ref[k];
				printf("#%d/%d, ", pruner->qnodes[idx].root, ref);
			}
			printf("max=%d] \n", pruner->backrefs[iid].max);
		}
	}
}

void math_pruner_print_stats(struct math_pruner *pruner)
{
	printf("%s=%d, %s=%d\n",
		STRVAR_PAIR(pruner->n_dropped_nodes),
		STRVAR_PAIR(pruner->n_dropped_iters)
	);
}