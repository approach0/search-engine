#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "example.h"

int example_invlist_empty(struct example_invlist *inv)
{
	return inv->len == 0;
}

example_invlist_iter_t example_invlist_iterator(struct example_invlist *inv)
{
	example_invlist_iter_t iter = malloc(sizeof *iter);
	iter->inv = inv;
	iter->idx = 0;
	return iter;
}

void example_invlist_iter_free(example_invlist_iter_t iter)
{
	free(iter);
}

uint64_t example_invlist_iter_cur(example_invlist_iter_t iter)
{
	if (iter->idx < iter->inv->len)
		return iter->inv->dat[iter->idx];
	else
		return UINT64_MAX;
}

int example_invlist_iter_next(example_invlist_iter_t iter)
{
	if (iter->idx < iter->inv->len)
		iter->idx += 1;

	return (iter->idx < iter->inv->len);
}

int example_invlist_iter_skip(example_invlist_iter_t iter, uint64_t to)
{
	do {
		uint64_t cur = example_invlist_iter_cur(iter);
		if (cur == UINT64_MAX)
			break;
		else if (cur >= to)
			return 1;

	} while (example_invlist_iter_next(iter));

	return 0;
}

size_t example_invlist_iter_read(example_invlist_iter_t iter,
                                 void *dest, size_t sz)
{
	if (iter->idx < iter->inv->len) {
		memcpy(dest, iter->inv->score + iter->idx, sz);
		return sz;
	} else {
		return 0;
	}
}
