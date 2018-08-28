#include <stdio.h>
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

struct postmerger_iterator
postmerger_iterator(struct postmerger *pm)
{
	struct postmerger_iterator iter;
	iter.size = pm->n_po;
	for (int i = 0; i < iter.size; i++) {
		iter.map[i] = i;
	}
	iter.min = postmerger_iter_min(pm, &iter);
	return iter;
}

int
postmerger_iter_next(struct postmerger *pm,
                     struct postmerger_iterator *iter)
{
	int last_round = (iter->min == UINT64_MAX);
	iter->min = postmerger_iter_min(pm, iter);
	return !last_round;
}

void
postmerger_iter_remove(struct postmerger_iterator *iter, int i)
{
	for (int j = i; j < iter->size - 1; j++) {
		iter->map[j] = iter->map[j + 1];
	}
	iter->size -= 1;
}
