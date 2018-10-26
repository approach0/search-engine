#include <stdio.h>
#include <stdlib.h>
#include "postmerger.h"

void postmerger_init(struct postmerger *pm)
{
	pm->n_po = 0;
}

int postmerger_empty(struct postmerger *pm)
{
	return (pm->n_po == 0);
}

static uint64_t
postmerger_iter_min(struct postmerger *pm,
                    struct postmerger_iterator *iter)
{
	uint64_t min = UINT64_MAX;
	for (int i = 0; i < iter->size; i++) {
		uint64_t cur = postmerger_iter_call(pm, iter, cur, i);
		if (cur < min) min = cur;
	}

	return min;
}

postmerger_iter_t
postmerger_iterator(struct postmerger *pm)
{
	struct postmerger_iterator *iter;
	iter = malloc(sizeof(struct postmerger_iterator));
	iter->size = pm->n_po;
	for (int i = 0; i < iter->size; i++) {
		iter->map[i] = i;
	}
	iter->pm  = pm;
	/* iter->min should be the last one after other
	 * members of `postmerger_iterator' are ready. */
	iter->min = postmerger_iter_min(iter->pm, iter);
	return iter;
}

void postmerger_iter_free(postmerger_iter_t iter)
{
	free(iter);
}

int postmerger_iter_next(postmerger_iter_t iter)
{
	int last_round = (iter->min == UINT64_MAX);
	if (last_round) {
		return 0;
	} else {
		iter->min = postmerger_iter_min(iter->pm, iter);
		return 1;
	}
}

void
postmerger_iter_remove(struct postmerger_iterator *iter, int i)
{
	for (int j = i; j < iter->size - 1; j++) {
		iter->map[j] = iter->map[j + 1];
	}
	iter->size -= 1;
}
