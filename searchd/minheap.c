#include <stdlib.h>
#include <stdio.h>
#include "minheap.h"

struct heap heap_create(uint32_t vol)
{
	struct heap h;
	h.array = calloc(vol, sizeof(void*));
	h.volume = vol;
	h.end = 0;
	h.ltf = NULL;

	return h;
}

void heap_set_callbk(struct heap *h, heap_lt_fun fun)
{
	h->ltf = fun;
}

void heap_destory(struct heap *h)
{
	free(h->array);
}

bool heap_full(struct heap *h)
{
	return (h->end == h->volume);
}

uint32_t heap_size(struct heap *h)
{
	return h->end;
}

void heap_push(struct heap *h, void *ptr)
{
	h->array[h->end ++] = ptr;
}

void *heap_top(struct heap *h)
{
	return h->array[0];
}

static __inline uint32_t right_son(uint32_t i)
{
	return (i << 1) + 2;
}

static
void _heap_print_tr(struct heap *h, uint32_t i,
                    uint32_t depth, heap_pr_fun fun)
{
	uint32_t j;
	(*fun)(h->array[i], i, depth);
	printf("\n");

	j = right_son(i);
	if (j < h->end)
		_heap_print_tr(h, j, depth + 1, fun);

	j = j - 1; /* left son */
	if (j < h->end)
		_heap_print_tr(h, j, depth + 1, fun);
}

void heap_print_tr(struct heap *h, heap_pr_fun fun)
{
	if (h->end)
		_heap_print_tr(h, 0, 0, fun);
}

void heap_print_arr(struct heap *h, heap_pr_fun fun)
{
	uint32_t i;
	for (i = 0; i < h->volume; i++) {
		if (i == h->end)
			printf("|       ");

		if (NULL == h->array[i])
			printf("nil     ");
		else
			(*fun)(h->array[i], i, 0);
	}

	printf("\n");
}

static __inline
void swap(struct heap *h, uint32_t i, uint32_t j)
{
	void *tmp = h->array[i];
	h->array[i] = h->array[j];
	h->array[j] = tmp;
}

static __inline
uint32_t min_son(struct heap *h, uint32_t i)
{
	uint32_t j, min = i;

	j = right_son(i);
	if (j < h->end)
		if ((*h->ltf)(h->array[j], h->array[min]))
			min = j;

	j = j - 1; /* left son */
	if (j < h->end) {
		if ((*h->ltf)(h->array[j], h->array[min]))
			min = j;
	}

	return min;
}

static __inline
void min_shift_down(struct heap *h, uint32_t i)
{
	uint32_t min;

	while (1) {
		min = min_son(h, i);

		if (min != i) {
			swap(h, min, i);
			i = min;
		} else {
			break;
		}
	}
}

void minheap_heapify(struct heap *h)
{
	uint32_t i = h->end >> 1;

	if (i)
		i --;
	else
		return;

	/* for each i that is non-leaf */
	while (1) {
		min_shift_down(h, i);

		if (i == 0)
			break;
		else
			i --;
	}
}

void minheap_delete(struct heap *h, uint32_t i)
{
	if (i < h->end) {
		h->end --;
		swap(h, i, h->end);
		min_shift_down(h, i);
	}
}

void minheap_replace(struct heap *h, uint32_t i, void *ptr)
{
	h->array[i] = ptr;
	min_shift_down(h, i);
}

void minheap_sort(struct heap *h)
{
	while (h->end)
		minheap_delete(h, 0);
}

void heap_sort_desc(struct heap *h)
{
	minheap_heapify(h);
	minheap_sort(h);
}

void minheap_insert(struct heap *h, void *ptr)
{
	uint32_t j, i = h->end;
	h->array[i] = ptr;
	h->end ++;

	while (i) {
		/* shift up... */
		j = (i - 1) >> 1; /* father */
		if ((*h->ltf)(h->array[i], h->array[j]))
			swap(h, i, j);
		i = j;
	}
}
