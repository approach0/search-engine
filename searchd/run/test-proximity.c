#include "term-index/term-index.h"
#include "proximity.h"

#define INIT_ARR(_name) \
	{sizeof(_name) / sizeof(position_t), _name, 0}

position_t test1()
{
	position_t arr1[] = {5, 8, 10, 19};
	position_t arr2[] = {1, 4, 9};
	position_t arr3[] = {2, 3, 6, 7, 11, 15};

	prox_input_t input[3] = {
		INIT_ARR(arr1),
		INIT_ARR(arr2),
		INIT_ARR(arr3)
	};

	return prox_min_dist(input, 3);
}

position_t test2()
{
	position_t arr1[] = {8, 9, 11, 12, 13};
	position_t arr2[] = {1, 2, 3, 4};
	position_t arr3[] = {16, 17};

	prox_input_t input[3] = {
		INIT_ARR(arr1),
		INIT_ARR(arr2),
		INIT_ARR(arr3)
	};

	return prox_min_dist(input, 3);
}

position_t test3()
{
	position_t arr1[] = {500, 512};

	prox_input_t input[1] = {
		INIT_ARR(arr1)
	};

	return prox_min_dist(input, 1);
}

int main()
{
	printf("=== test1 ===\n");
	printf("res = %u.\n\n", test1());

	printf("=== test2 ===\n");
	printf("res = %u.\n\n", test2());

	printf("=== test3 ===\n");
	printf("res = %u.\n\n", test3());
}
