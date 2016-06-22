#include <stdio.h>
#include "minheap.h"

#define COL_BEG "\033[1m\033[34m"
#define COL_END "\033[0m"

bool test_less_than(void* ele0, void* ele1)
{
	uint32_t dat0, dat1;

	dat0 = *(uint32_t *)ele0;
	dat1 = *(uint32_t *)ele1;

	return dat0 < dat1;
}

void test_print(void* ele, uint32_t i, uint32_t depth)
{
	uint32_t j;
	for (j = 0; j < depth; j++)
		printf("  ");

	printf("[%d]:%-3d ", i, *(uint32_t *)ele);
}

int main(void)
{
	uint32_t cnt, i, data[] =
	                    {14, 2, 22, 13, 23, 10, 90, 36, 108, 12,
	                      9, 91, 1, 51, 11, 3, 15, 80, 3, 78, 53,
	                      5, 12, 21, 65, 70, 4};
	const uint32_t data_len = sizeof(data)/sizeof(uint32_t);
	struct heap heap = heap_create(data_len + 5);

	printf(COL_BEG "push data...\n" COL_END);
	for (i = 0; i < data_len; i++)
		if (!heap_full(&heap))
			heap_push(&heap, &data[i]);

	printf("data len = %u, heap size = %u.\n", data_len,
	       heap_size(&heap));

	printf(COL_BEG "heap tree:\n" COL_END);
	heap_print_tr(&heap, &test_print);

	printf(COL_BEG "heap array:\n" COL_END);
	heap_print_arr(&heap, &test_print);

	heap_set_callbk(&heap, &test_less_than);

	printf(COL_BEG "after heapify:\n" COL_END);
	minheap_heapify(&heap);
	heap_print_tr(&heap, &test_print);

	cnt = 0;
	printf(COL_BEG "ranking emulation...:\n" COL_END);
	while (cnt < 100) {
		i = (i + 1) % data_len;
		if (!heap_full(&heap)) {
			printf("insert %d\n", data[i]);
			minheap_insert(&heap, &data[i]);
		} else {
			void *top = heap_top(&heap);
			if (test_less_than(top, &data[i])) {
				printf("replace with %d\n", data[i]);
				minheap_delete(&heap, 0);
				minheap_insert(&heap, &data[i]);
			}
		}
		cnt ++;
	}

	printf(COL_BEG "a heavy heap tree now:\n" COL_END);
	heap_print_tr(&heap, &test_print);

	minheap_sort(&heap);
	printf(COL_BEG "heap array after min-heap sort:\n" COL_END);
	heap_print_arr(&heap, &test_print);

	heap_destory(&heap);

	printf(COL_BEG "a new heap...\n" COL_END);
	heap = heap_create(data_len + 5);
	heap_set_callbk(&heap, &test_less_than);

	for (i = 0; i < data_len; i++)
		if (!heap_full(&heap))
			heap_push(&heap, &data[i]);

	printf(COL_BEG "heap array:\n" COL_END);
	heap_print_arr(&heap, &test_print);

	heap_sort_desc(&heap);
	printf(COL_BEG "heap array after heap sort:\n" COL_END);
	heap_print_arr(&heap, &test_print);

	heap_print_tr(&heap, &test_print);
	heap_destory(&heap);

	printf(COL_BEG "sort a heap with one element...\n" COL_END);

	heap = heap_create(data_len + 5);
	heap_set_callbk(&heap, &test_less_than);
	heap_push(&heap, &data[0]);
	heap_print_tr(&heap, &test_print);
	heap_sort_desc(&heap);

	heap_destory(&heap);
	return 0;
}
