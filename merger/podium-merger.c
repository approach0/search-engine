#include <stdlib.h>
#include <stdio.h>
#include <float.h> /* for FLT_MAX */
#include "common/common.h" /* for MAX() */
#include "mergers.h"

int pd_merger_lift_up_pivot(struct pd_merger *m, float threshold,
                            merger_upp_relax_fun relax, void *arg)
{
	for (int i = m->pivot; i >= 0; i--) {
		float sum = m->acc_upp[i];
		float max = m->acc_max[i];
		if (relax(arg, max + sum) > threshold) {
			m->pivot = i;
			break;
		}
	}

	return m->pivot;
}

static void pd_merger_update_acc_values(struct pd_merger *m)
{
	float sum = 0, max = -FLT_MAX;
	for (int i = m->size - 1; i >= 0; i--) {
		float upp = merger_map_prop(m, upp, i);
		float pd_upp = merger_map_prop(m, pd_upp, i);
		sum += upp;
		max = MAX(max, pd_upp);
		m->acc_upp[i] = sum;
		m->acc_max[i] = max;
	}
}

int pd_merger_map_remove(struct pd_merger *m, int i)
{
	m->size -= 1;
	if (m->pivot >= i)
		m->pivot -= 1;

	/* fill the gap */
	for (int j = i; j < m->size; j++)
		m->map[j] = m->map[j + 1];

	pd_merger_update_acc_values(m);
	return i - 1;
}

int pd_merger_iter_follow(struct pd_merger *m, int iid)
{
	uint64_t cur = MERGER_ITER_CALL(m, cur, iid);
	int left = (cur != UINT64_MAX);
	if (cur < m->min)
		left = MERGER_ITER_CALL(m, skip, iid, m->min);

	return left;
}

static void pd_merger_iter_sort(struct pd_merger *m)
{
	// TODO: replace bubble sort with quick-sort.
	for (int i = 0; i < m->size; i++) {
		float max_key = merger_map_prop(m, sortby, i);
		for (int j = i + 1; j < m->size; j++) {
			float key = merger_map_prop(m, sortby, j);
			if (key > max_key) {
				max_key = key;
				/* swap */
				int tmp;
				tmp = m->map[i];
				m->map[i] = m->map[j];
				m->map[j] = tmp;
			}
		}
	}
}

uint64_t pd_merger_min(struct pd_merger *m)
{
	uint64_t cur, min = UINT64_MAX;
	for (int i = 0; i <= m->pivot; i++) {
		cur = merger_map_call(m, cur, i);
		if (cur < min) min = cur;
	}
	return min;
}

struct pd_merger *pd_merger_iterator(merge_set_t* set)
{
	struct pd_merger *m = malloc(sizeof *m);
	m->set = *set;

	for (int i = 0; i < set->n; i++)
		m->map[i] = i;

	m->size = set->n;
	m->pivot = set->n - 1;
	m->min = pd_merger_min(m);

	pd_merger_iter_sort(m);
	pd_merger_update_acc_values(m);
	return m;
}

void pd_merger_iter_free(struct pd_merger *m)
{
	free(m);
}

int pd_merger_iter_next(struct pd_merger *m)
{
	if (m->min == UINT64_MAX)
		return 0;

	for (int i = 0; i <= m->pivot; i++) {
		uint64_t cur = merger_map_call(m, cur, i);
		if (cur == m->min) {
			int left = merger_map_call(m, next, i);
			if (!left)
				i = pd_merger_map_remove(m, i);
		}
	}

	m->min = pd_merger_min(m);
	return 1;
}

void pd_merger_iter_print(struct pd_merger* m, merger_keyprint_fun keyprint)
{
	/* header first */
	printf("%c | %5s | %-6s %5s | %-6s %5s [%3s]\n", 'F',
		"score", "accmax", "pdupp", "accupp", "upp", "inv");

	for (int i = 0; i < m->size; i++) {
		int invi = m->map[i];
		int pivot = m->pivot;
		float acc_upp = m->acc_upp[i];
		float acc_max = m->acc_max[i];
		float upp = merger_map_prop(m, upp, i);
		float pd_upp = merger_map_prop(m, pd_upp, i);
		uint64_t cur = merger_map_call(m, cur, i);
		char flag = ' ';
		if (i == pivot) flag = 'P'; else if (i > pivot) flag = 'S';
		printf("%c | %5.2f | %-6.2f %5.2f | %-6.2f %5.2f [%3d] ",
			flag, pd_upp + upp, acc_max, pd_upp, acc_upp, upp, invi);
		if (NULL != keyprint)
			keyprint(cur);
		else
			printf("#%lu", cur);
		if (cur == m->min)
			printf(" <- Candidate");
		printf("\n");
	}
}
