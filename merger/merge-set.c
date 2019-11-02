#include "merge-set.h"

int merger_set_empty(struct merge_set *set)
{
	return set->n == 0;
}
