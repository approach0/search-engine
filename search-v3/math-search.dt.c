#include "config.h"
#include "math-search.h"

struct math_l2_invlist
*math_l2_invlist(math_index_t index, const char* tex, float *threshold)
{
	/* initialize math query */
	struct math_qry mq;
	if (0 != math_qry_prepare(index, tex, &mq)) {
		/* parsing failed */
		math_qry_release(&mq);
		return NULL;
	}

	/* allocate memory */
	struct math_l2_invlist *ret = malloc(sizeof *ret);

	/* save math query structure */
	ret->mq = mq;

	/* initialize scorer */
	math_score_precalc(&ret->msf);

	/* copy threshold address */
	ret->threshold = threshold;
	return ret;
}

void math_l2_invlist_free(struct math_l2_invlist *inv)
{
	math_qry_release(&inv->mq);
	free(inv);
}

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator   ms_merger_iterator
#define merger_set_iter_next  ms_merger_iter_next
#define merger_set_iter_free  ms_merger_iter_free
#define merger_set_map_follow ms_merger_map_follow

math_l2_invlist_iter_t math_l2_invlist_iterator(struct math_l2_invlist *inv)
{
	if (merger_set_empty(&inv->mq.merge_set))
		return NULL;

	math_l2_invlist_iter_t l2_iter = malloc(sizeof *l2_iter);

	l2_iter->n_qnodes       = inv->mq.n_qnodes;
	l2_iter->ipf            = inv->mq.ipf;
	l2_iter->msf            = &inv->msf;
	l2_iter->merge_iter     = merger_set_iterator(&inv->mq.merge_set);
	l2_iter->future_docID   = 0;
	l2_iter->last_threshold = -1.f;
	l2_iter->threshold      = inv->threshold;

	l2_iter->pruner = math_pruner_init(&inv->mq, &inv->msf, *inv->threshold);

	/* run level-2 next() **twice** to forward the current
	 * ID from zero to future_docID (also zero initially)
	 * and then forward to the first item docID. */
	if (0 != math_l2_invlist_iter_next(l2_iter))
		math_l2_invlist_iter_next(l2_iter);

	return l2_iter;
}

void math_l2_invlist_iter_free(math_l2_invlist_iter_t l2_iter)
{
	if (l2_iter->pruner)
		math_pruner_free(l2_iter->pruner);

	merger_set_iter_free(l2_iter->merge_iter);
	free(l2_iter);
}

uint64_t math_l2_invlist_iter_cur(math_l2_invlist_iter_t l2_iter)
{
	if (l2_iter->item.docID == UINT32_MAX)
		return UINT64_MAX;
	else
		return l2_iter->item.docID;
}

static inline int
hit_nodes(struct math_pruner *pruner, int N, merger_set_iter_t iter, int *out)
{
	int n = 0, max_n = N;

	/* create a simple bitmap */
	int hits[max_n];
	memset(hits, 0, sizeof hits);

	/* find all hit node indexs */
	for (int i = 0; i <= iter->pivot; i++) {
		uint64_t cur = merger_map_call(iter, cur, i);
		if (cur == iter->min) {
			struct math_pruner_backref brefs;
			int iid = iter->map[i];
			brefs = pruner->backrefs[iid];
			/* for all nodes linked to a hit inverted list */
			for (int j = 0; j < brefs.cnt; j++) {
				int idx = brefs.idx[j];
				if (!hits[idx]) {
					hits[idx] = 1;
					out[n++] = idx;
				}
			}
		}
	}

	return n;
}

static inline float
struct_score(merger_set_iter_t iter, struct math_pruner_qnode *qnode,
	struct math_score_factors *msf, float *ipf, float best, float threshold)
{
	float estimate, score = 0, leftover = qnode->sum_ipf;
	/* for each sector tree under qnode */
	for (int i = 0; i < qnode->n; i++) {
		/* sector tree variables */
		int iid = qnode->invlist_id[i];
		int ref = qnode->secttr_w[i];

		/* skip set iterators must follow up */
		if (i > iter->pivot)
			if (!merger_set_map_follow(iter, i))
				goto skip;

		uint64_t cur = MERGER_ITER_CALL(iter, cur, iid);
		if (cur == iter->min) {
			/* read hit inverted list item */
			struct math_invlist_item item;
			MERGER_ITER_CALL(iter, read, iid, &item, sizeof(item));
			/* accumulate preceise partial score */
			score += MIN(ref, item.sect_width) * ipf[iid];
		}
skip:
		leftover = leftover - ref * ipf[iid];
		estimate = score + leftover; /* new score estimation */
#ifndef MATH_PRUNING_STRATEGY_NONE
		if (estimate <= best || math_score_upp(msf, estimate) <= threshold)
			return 0;
#endif
	}

	return score;
}

int math_l2_invlist_iter_next(math_l2_invlist_iter_t l2_iter)
{
	struct math_pruner *pruner = l2_iter->pruner;
	merger_set_iter_t iter = l2_iter->merge_iter;
	float threshold = *l2_iter->threshold;

#ifndef MATH_PRUNING_STRATEGY_NONE
	/* handle threshold update */
	if (threshold != l2_iter->last_threshold) {
		if (math_pruner_update(pruner, threshold))
			printf("[node dropped]\n");

		math_pruner_iters_drop(pruner, iter);

#if defined(MATH_PRUNING_STRATEGY_MAXREF)
		math_pruner_iters_sort_by_maxref(pruner, iter);

#elif defined(MATH_PRUNING_STRATEGY_GBP_NUM)
		math_pruner_iters_gbp_assign(pruner, iter, 0);

#elif defined(MATH_PRUNING_STRATEGY_GBP_LEN)
		math_pruner_iters_gbp_assign(pruner, iter, 1);

#else
	#error("no math pruning strategy specified.")
#endif

		l2_iter->last_threshold = threshold;

		/* if there is no element in requirement set */
		if (iter->pivot < 0)
			goto terminated;
	}
#endif

#ifdef DEBUG_MATH_SEARCH
	printf("[math merge iteration]\n");
	math_pruner_print(pruner);
	ms_merger_iter_print(iter, NULL);
	printf("\n");
#endif

	/* safe guard */
	if (l2_iter->future_docID == UINT32_MAX)
		goto terminated;

	/* merge to the next */
	float *ipf = l2_iter->ipf;
	struct math_score_factors *msf = l2_iter->msf;
	int n_max_nodes = l2_iter->n_qnodes;
	do {
		/* set current candidate */
		uint32_t cur_docID = key2doc(iter->min);

		/* test the termination of level-2 merge iteration */
		if (cur_docID != l2_iter->future_docID) {
			/* passed through all maths in future_docID. */
			l2_iter->item.docID = l2_iter->future_docID;
			l2_iter->future_docID = cur_docID;
			return 1;
		}

		/* get hits query nodes of current candidate */
		int out[n_max_nodes];
		int n_hit_nodes = hit_nodes(pruner, n_max_nodes, iter, out);

		/* calculate structural score */
		float t, best = 0;
		int best_qnode = 0, best_dnode = 0;
		for (int i = 0; i < n_hit_nodes; i++) {
			struct math_pruner_qnode *qnode = pruner->qnodes + out[i];
#ifndef MATH_PRUNING_STRATEGY_NONE
			if (qnode->sum_ipf <= best)
				continue;
#endif
			/* get structural score of matched tree rooted at qnode */
			t = struct_score(iter, qnode, msf, ipf, best, threshold);
			if (t > best) {
				best = t;
				best_qnode = qnode->root;
				best_dnode = key2rot(iter->min);
			}
		}

		/* if structural score is non-zero */
		if (best > 0) {
			/* TODO: calculate symbol score here */
			(void)best_qnode;
			(void)best_dnode;

			/* take max expression score as math score for this docID */
			if (best > l2_iter->item.score) {
				/* update the returning item values for current docID */
				l2_iter->item.score = best;
				l2_iter->item.n_occurs = 1;
				l2_iter->item.occur[0] = key2exp(iter->min);
#ifdef DEBUG_MATH_SEARCH
				printf("[doc#%u exp#%u match %d<->%d]: %.2f\n", cur_docID,
					key2exp(iter->min), best_qnode, best_dnode, best);
#endif
			}
		}

	} while (merger_set_iter_next(iter));

terminated: /* when iterator reaches the end */
	l2_iter->item.docID = UINT32_MAX;
	l2_iter->item.score = 0.f;
	l2_iter->item.n_occurs = 0;
	return 0;
}

size_t
math_l2_invlist_iter_read(math_l2_invlist_iter_t l2_iter, void *dst, size_t sz)
{
	memcpy(dst, &l2_iter->item, sizeof(struct math_l2_iter_item));
	return sizeof(struct math_l2_iter_item);
}

float math_l2_invlist_iter_upp(math_l2_invlist_iter_t l2_iter)
{
	struct math_pruner* pruner = l2_iter->pruner;
	struct math_score_factors *msf = l2_iter->msf;

	float max_sum_ipf = 0;
	for (int i = 0; i < pruner->n_qnodes; i++) {
		struct math_pruner_qnode *qnode = pruner->qnodes + i;
		if (max_sum_ipf < qnode->sum_ipf)
			max_sum_ipf = qnode->sum_ipf;
	}
	return math_score_upp(msf, max_sum_ipf);
}
