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
	struct math_l2_invlist *ret = calloc(1, sizeof(ret));

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
}

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator  ms_merger_iterator
#define merger_set_iter_next ms_merger_iter_next
#define merger_set_iter_free ms_merger_iter_free

math_l2_invlist_iter_t math_l2_invlist_iterator(struct math_l2_invlist *inv)
{
	if (merger_set_empty(&inv->mq.merge_set))
		return NULL;

	math_l2_invlist_iter_t l2_iter = malloc(sizeof *l2_iter);

	l2_iter->merge_iter = merger_set_iterator(&inv->mq.merge_set);
	l2_iter->future_docID = 0;
	l2_iter->last_threshold = -1.f;
	l2_iter->threshold = inv->threshold;

	l2_iter->pruner = math_pruner_init(&inv->mq, &inv->msf, *inv->threshold);

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

	/* make shortcut for current document ID */
	uint32_t *cur_docID = &l2_iter->item.docID;

	/* safe guard */
	if (l2_iter->future_docID == UINT32_MAX)
		goto terminated;

	/* reset occurred position */
	l2_iter->item.n_occurs = 0;

	/* merge to the next */
	do {
		/* set current candidate */
		*cur_docID = key2doc(iter->min);

		/* test the termination of level-2 merge iteration */
		if (*cur_docID != l2_iter->future_docID) {
			/* passed through all maths in future_docID. */
			uint32_t save = *cur_docID;
			*cur_docID = l2_iter->future_docID;
			l2_iter->future_docID = save;
			return 1;
		}

		/* scoring the current candidate */
	} while (merger_set_iter_next(iter));

terminated:
	l2_iter->item.docID = UINT32_MAX;
	l2_iter->item.score = 0.f;
	return 0;
}

size_t
math_l2_invlist_iter_read(math_l2_invlist_iter_t l2_iter, void *dst, size_t sz)
{
	memcpy(dst, &l2_iter->item, sizeof(struct math_l2_iter_item));
	return sizeof(struct math_l2_iter_item);
}
