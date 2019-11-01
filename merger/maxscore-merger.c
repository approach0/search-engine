#include <stdlib.h>
#include "mergers.h"

float no_upp_relax(void *_, float upp)
{
	return upp;
}

int ms_merger_lift_up_pivot(struct ms_merger *m, float threshold,
                            upp_relax_callbk relax, void *arg)
{
	for (int i = m->pivot; i >= 0; i--) {
		float sum = m->acc_upp[i];
		if (relax(arg, sum) > threshold) {
			m->pivot = i;
			break;
		}
	}

	return m->pivot;
}

void ms_merger_update_acc_upp(struct ms_merger *m)
{
	float sum = 0;
	for (int i = m->size - 1; i >= 0; i--) {
		float upp = merger_map_upp(m, i);
		sum += upp;
		m->acc_upp[i] = sum;
	}
}

void ms_merger_map_remove(struct ms_merger *m, int i)
{
	m->size -= 1;
	if (m->pivot >= i)
		m->pivot -= 1;

	/* fill the gap */
	for (int j = i; j < m->size; j++)
		m->map[j] = m->map[j + 1];

	ms_merger_update_acc_upp(m);
}

int ms_merger_skipset_follow(struct ms_merger *m)
{
	int follows = 0;
	for (int i = m->pivot + 1; i < m->size; i++) {
		uint64_t cur = merger_map_call(m, cur, i);

		if (cur < m->min) {
			int left = merger_map_call(m, skip, i, m->min);
			if (!left) {
				ms_merger_map_remove(m, i);
				i --;
			}

			follows ++;
		}
	}

	return follows;
}

void ms_merger_iter_sort_by_upp(struct ms_merger *m)
{
	// TODO: replace bubble sort with quick-sort.
	for (int i = 0; i < m->size; i++) {
		float max_upp = merger_map_upp(m, i);
		for (int j = i + 1; j < m->size; j++) {
			float upp = merger_map_upp(m, j);
			if (upp > max_upp) {
				max_upp = upp;
				/* swap */
				int tmp;
				tmp = m->map[i];
				m->map[i] = m->map[j];
				m->map[j] = tmp;
			}
		}
	}

	ms_merger_update_acc_upp(m);
}

uint64_t ms_merger_min(struct ms_merger *m)
{
	uint64_t cur, min = UINT64_MAX;
	for (int i = 0; i <= m->pivot; i++) {
		cur = merger_map_call(m, cur, i);
		if (cur < min) min = cur;
	}
	return min;
}

struct ms_merger *ms_merger_iterator(merge_set_t* set)
{
	struct ms_merger *m = malloc(sizeof *m);
	m->set = *set;

	for (int i = 0; i < set->n; i++)
		m->map[i] = i;
	ms_merger_iter_sort_by_upp(m); /* acc_upp is also updated */

	m->size = set->n;
	m->pivot = set->n - 1;
	m->min = ms_merger_min(m);

	return m;
}

int ms_merger_empty(struct ms_merger* m)
{
	return (m->pivot < 0);
}

void ms_merger_iter_free(struct ms_merger *m)
{
	free(m);
}

int ms_merger_iter_next(struct ms_merger *m)
{
	if (m->min == UINT64_MAX)
		return 0;

	for (int i = 0; i <= m->pivot; i++) {
		uint64_t cur = merger_map_call(m, cur, i);
		if (cur == m->min) {
			int left = merger_map_call(m, next, i);
			if (!left) {
				ms_merger_map_remove(m, i);
				i --;
			}
		}
	}

	m->min = ms_merger_min(m);
	return 1;
}
