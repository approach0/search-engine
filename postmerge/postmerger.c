#include <stdio.h>
#include <stdlib.h>
#include "postmerger.h"

int postmerger_empty(struct postmerger_postlists *pols)
{
	return (pols->n == 0);
}

static uint64_t
postmerger_min(struct postmerger *pm)
{
	uint64_t cur, min = UINT64_MAX;
	for (int i = 0; i < pm->size; i++) {
		cur = postmerger_iter_call(pm, cur, i);
		if (cur < min) min = cur;
	}

	return min;
}

postmerger_iter_t
postmerger_iterator(struct postmerger_postlists *pols)
{
	struct postmerger *pm;
	pm = malloc(sizeof(struct postmerger));
	pm->size = pols->n;
	for (int i = 0; i < pols->n; i++) {
		pm->po[i]   = pols->po[i];
		pm->map[i]  = i;
		pm->iter[i] = POSTMERGER_POSTLIST_FUN(pm, get_iter, i, pols->po[i].po);
	}
	/* set `min' when iterators are ready to read. */
	pm->min = postmerger_min(pm);
	return pm;
}

void postmerger_iter_free(struct postmerger *pm)
{
	for (int i = 0; i < pm->size; i++)
		POSTMERGER_POSTLIST_CALL(pm, del_iter, i);
	free(pm);
}

int postmerger_iter_next(struct postmerger *pm)
{
	if (pm->min == UINT64_MAX) {
		return 0;
	} else {
		/* actual forwarding left to explict calls,
		 * here only update the min value. */
		pm->min = postmerger_min(pm);
		return 1;
	}
}

void postmerger_iter_remove(struct postmerger *pm, int i)
{
	pm->size -= 1;
	for (int j = i; j < pm->size; j++)
		pm->map[j] = pm->map[j + 1];
}
