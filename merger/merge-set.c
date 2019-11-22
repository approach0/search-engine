#include "merge-set.h"

int merger_set_empty(struct merge_set *set)
{
	return set->n == 0;
}

/*
 * Empty posting list calls
 */
uint64_t empty_invlist_cur(void *iter)
{
	return UINT64_MAX;
}

int empty_invlist_next(void *iter)
{
	return 0;
}

int empty_invlist_skip(void *iter, uint64_t target)
{
	return 0;
}

size_t empty_invlist_read(void *iter, void *dest, size_t sz)
{
	return 0;
}
