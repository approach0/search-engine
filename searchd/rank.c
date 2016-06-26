#include <stdlib.h>
#include <stdio.h>
#include "term-index/term-index.h" /* for doc_id_t */
#include "rank.h"

static bool ri_less_than(void* ele0, void* ele1)
{
	struct rank_item *ri0 = (struct rank_item *)ele0;
	struct rank_item *ri1 = (struct rank_item *)ele1;

	return ri0->score < ri1->score;
}

void rank_init(struct rank_set *rs, uint32_t size,
               uint32_t n_terms, uint32_t n_frmls)
{
	rs->heap = heap_create(size);
	rs->n_terms = n_terms;
	rs->n_frmls = n_frmls;
	rs->n_items = 0;
	heap_set_callbk(&rs->heap, &ri_less_than);
}

void rank_cram(struct rank_set *rs, doc_id_t docID, float score,
               uint32_t hit_terms, char **terms, frml_id_t **frmlIDs)
{
	struct rank_item *ri;
	void *top;

	ri = malloc(sizeof(struct rank_item));
	ri->docID = docID;
	ri->terms = terms;
	ri->hit_terms = hit_terms;
	ri->frmlIDs = frmlIDs;
	ri->score = score;

	if (!heap_full(&rs->heap)) {
		minheap_insert(&rs->heap, ri);
		rs->n_items ++;
	} else {
		top = heap_top(&rs->heap);

		if (ri_less_than(top, ri)) {
			free(top);
			minheap_delete(&rs->heap, 0);

			minheap_insert(&rs->heap, ri);
		} else {
			free(ri);
		}
	}
}

void rank_sort(struct rank_set *rs)
{
	minheap_sort(&rs->heap);
}

void rank_uninit(struct rank_set *rs)
{
	uint32_t i;
	for (i = 0; i < rs->n_items; i++)
		free(rs->heap.array[i]);

	heap_destory(&rs->heap);
}

static void print(void* ele, uint32_t i, uint32_t depth)
{
	struct rank_item *ri = (struct rank_item*)ele;
	uint32_t j;
	for (j = 0; j < depth; j++)
		printf("  ");

	printf("[%d]:doc#%u(%.2f) ", i, ri->docID, ri->score);
}

void rank_print(struct rank_set *rs)
{
	printf("heap n_items = %u\n", rs->n_items);
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

	*total_page = div_ceil(rs->n_items, res_per_page);

	if (page_start >= *total_page)
		return win;

	win.from = page_start * res_per_page;
	win.to   = (page_start + 1) * res_per_page;

	if (win.to > rs->n_items)
		win.to = rs->n_items;

	return win;
}

uint32_t rank_window_foreach(struct rank_wind *win,
                             rank_wind_callbk fun, void *arg)
{
	uint32_t i, cnt = 0;
	struct heap *h = &win->rs->heap;

	for (i = win->from; i < win->to; i++) {
		(*fun)(win->rs, (struct rank_item*)h->array[i], cnt, arg);
		cnt ++;
	}

	return cnt;
}
