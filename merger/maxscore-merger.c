#include <stdlib.h>
#include <stdio.h>
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

int ms_merger_map_remove(struct ms_merger *m, int i)
{
	m->size -= 1;
	if (m->pivot >= i)
		m->pivot -= 1;

	/* fill the gap */
	for (int j = i; j < m->size; j++)
		m->map[j] = m->map[j + 1];

	ms_merger_update_acc_upp(m);
	return i - 1;
}

int ms_merger_map_follow(struct ms_merger *m, int i)
{
	uint64_t cur = merger_map_call(m, cur, i);
	int left = (cur != UINT64_MAX);
	if (cur < m->min)
		left = merger_map_call(m, skip, i, m->min);

	return left;
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

	m->size = set->n;
	m->pivot = set->n - 1;
	m->min = ms_merger_min(m);

	ms_merger_iter_sort_by_upp(m); /* acc_upp is also updated */
	return m;
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
			if (!left)
				i = ms_merger_map_remove(m, i);
		}
	}

	m->min = ms_merger_min(m);
	return 1;
}

void ms_merger_iter_print(struct ms_merger* m, keyprint_fun keyprint)
{
	for (int i = 0; i < m->size; i++) {
		int invi = m->map[i];
		int pivot = m->pivot;
		float acc_upp = m->acc_upp[i];
		float upp = merger_map_upp(m, i);
		uint64_t cur = merger_map_call(m, cur, i);
		char flag = ' ';
		if (i == pivot) flag = 'P'; else if (i > pivot) flag = 'S';
		printf("%c %-6.2f %5.2f [%3d] ", flag, acc_upp, upp, invi);
		if (NULL != keyprint)
			keyprint(cur);
		else
			printf("#%lu", cur);
		if (cur == m->min)
			printf(" <- Candidate");
		printf("\n");
	}
}
