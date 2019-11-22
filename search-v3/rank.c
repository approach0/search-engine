#include <stdlib.h>
#include <stdio.h>
#include "rank.h"
#include "config.h"

static bool score_less_than(void* ele0, void* ele1)
{
	struct rank_hit *hit0 = (struct rank_hit *)ele0;
	struct rank_hit *hit1 = (struct rank_hit *)ele1;

	return hit0->score < hit1->score;
}

void priority_Q_init(struct priority_Q *Q, uint32_t volume)
{
	Q->heap = heap_create(volume);
	Q->n_elements = 0;
	heap_set_callbk(&Q->heap, &score_less_than);
}

bool priority_Q_full(struct priority_Q* Q)
{
	return heap_full(&Q->heap);
}

uint32_t priority_Q_len(struct priority_Q* Q)
{
	return heap_size(&Q->heap);
}

float priority_Q_min_score(struct priority_Q *Q)
{
	void *top = heap_top(&Q->heap);
	return ((struct rank_hit *)top)->score;
}

struct rank_hit *priority_Q_top(struct priority_Q *Q)
{
	void *top = heap_top(&Q->heap);
	return (struct rank_hit *)top;
}

bool priority_Q_add_or_replace(struct priority_Q *Q, struct rank_hit *hit)
{
	/* insert this element or replace the top one in Q */
	if (!heap_full(&Q->heap)) {
		minheap_insert(&Q->heap, hit);
		Q->n_elements ++;
		return 1;

	} else {
		struct rank_hit *top = (struct rank_hit *)heap_top(&Q->heap);
		minheap_delete(&Q->heap, 0);
		minheap_insert(&Q->heap, hit);

		free(top->occur);
		free(top);
		return 0;
	}
}

void priority_Q_sort(struct priority_Q *Q)
{
	minheap_sort(&Q->heap);
}

void priority_Q_free(struct priority_Q *Q)
{
	uint32_t i;
	struct rank_hit *hit;

	for (i = 0; i < Q->n_elements; i++) {
		hit = (struct rank_hit *)Q->heap.array[i];
		free(hit->occur);
		free(hit);
	}

	heap_destory(&Q->heap);
}

static void print(void* ele, unsigned int i, unsigned int depth)
{
	struct rank_hit *hit = (struct rank_hit*)ele;
	for (unsigned int j = 0; j < depth; j++)
		printf("  ");

	printf("[%d]:doc#%u(%.2f) ", i, hit->docID, hit->score);
}

void priority_Q_print(struct priority_Q *Q)
{
	printf("priority Q elements: %u\n", Q->n_elements);
	printf("rank heap:\n");
	heap_print_tr(&Q->heap, &print);
	printf("rank array:\n");
	heap_print_arr(&Q->heap, &print);
	printf("\n");
}

static __inline
int div_ceil(int a, int b)
{
	int ret = a / b;
	if (a % b)
		return ret + 1;
	else
		return ret;
}

struct rank_wind
rank_wind_calc(ranked_results_t *r, int wind_start,
	int res_per_wind, int *n_wind)
{
	struct rank_wind wind = {r, 0, 0};
	*n_wind = 0;

	if (res_per_wind == 0)
		return wind;

	*n_wind = div_ceil(r->n_elements, res_per_wind);

	if (wind_start >= *n_wind)
		return wind;

	if (wind_start < 0) {
		/* return all results */
		wind.from = 0;
		wind.to   = r->n_elements;
	} else {
		/* return results at the range of a page */
		wind.from = wind_start * res_per_wind;
		wind.to   = (wind_start + 1) * res_per_wind;
	}

	if (wind.to > r->n_elements)
		wind.to = r->n_elements;

	return wind;
}

int
rank_wind_foreach(struct rank_wind *wind, rank_wind_callbk fun, void *arg)
{
	int i, cnt = 0;
	struct heap *h = &wind->results->heap;
	struct rank_result res;

	for (i = wind->from; i < wind->to; i++) {
		res.hit  = (struct rank_hit*)h->array[i];
		res.from = wind->from;
		res.to   = wind->to;
		res.cnt  = cnt;
		(*fun)(&res, arg);
		cnt ++;
	}

	return cnt;
}
