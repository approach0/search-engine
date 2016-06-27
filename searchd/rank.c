#include <stdlib.h>
#include <stdio.h>
#include "term-index/term-index.h" /* for doc_id_t */
#include "rank.h"

static bool hit_score_less_than(void* ele0, void* ele1)
{
	struct rank_hit *hit0 = (struct rank_hit *)ele0;
	struct rank_hit *hit1 = (struct rank_hit *)ele1;

	return hit0->score < hit1->score;
}

void rank_set_init(struct rank_set *rs, uint32_t volume)
{
	rs->heap = heap_create(volume);
	rs->n_hits = 0;
	heap_set_callbk(&rs->heap, &hit_score_less_than);
}

void rank_set_hit(struct rank_set *rs, doc_id_t docID, float score)
{
	struct rank_hit *hit;
	void *top;

	hit = malloc(sizeof(struct rank_hit));
	hit->docID = docID;

	if (!heap_full(&rs->heap)) {
		minheap_insert(&rs->heap, hit);
		rs->n_hits ++;
	} else {
		top = heap_top(&rs->heap);

		if (hit_score_less_than(top, hit)) {
			free(top);
			minheap_delete(&rs->heap, 0);

			minheap_insert(&rs->heap, hit);
		} else {
			free(hit);
		}
	}
}

void rank_sort(struct rank_set *rs)
{
	minheap_sort(&rs->heap);
}

void rank_set_free(struct rank_set *rs)
{
	uint32_t i;
	for (i = 0; i < rs->n_hits; i++)
		free(rs->heap.array[i]);

	heap_destory(&rs->heap);
}

static void print(void* ele, uint32_t i, uint32_t depth)
{
	struct rank_hit *hit = (struct rank_hit*)ele;
	uint32_t j;
	for (j = 0; j < depth; j++)
		printf("  ");

	printf("[%d]:doc#%u(%.2f) ", i, hit->docID, hit->score);
}

void rank_print(struct rank_set *rs)
{
	printf("heap n_hits = %u\n", rs->n_hits);
	printf("rank heap:\n");
	heap_print_tr(&rs->heap, &print);
	printf("rank array:\n");
	heap_print_arr(&rs->heap, &print);
	printf("\n");
}

static __inline
uint32_t div_ceil(uint32_t a, uint32_t b)
{
	uint32_t ret = a / b;
	if (a % b)
		return ret + 1;
	else
		return ret;
}

struct rank_wind rank_window_calc(struct rank_set *rs,
                                  uint32_t page_start,
                                  uint32_t res_per_page,
                                  uint32_t *total_page)
{
	struct rank_wind win = {rs, 0, 0};
	*total_page = 0;

	if (res_per_page == 0)
		return win;

	*total_page = div_ceil(rs->n_hits, res_per_page);

	if (page_start >= *total_page)
		return win;

	win.from = page_start * res_per_page;
	win.to   = (page_start + 1) * res_per_page;

	if (win.to > rs->n_hits)
		win.to = rs->n_hits;

	return win;
}

uint32_t rank_window_foreach(struct rank_wind *win,
                             rank_wind_callbk fun, void *arg)
{
	uint32_t i, cnt = 0;
	struct heap *h = &win->rs->heap;

	for (i = win->from; i < win->to; i++) {
		(*fun)(win->rs, (struct rank_hit*)h->array[i], cnt, arg);
		cnt ++;
	}

	return cnt;
}
