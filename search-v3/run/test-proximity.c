#include "mhook/mhook.h"
#include "proximity.h"

#define INIT_ARR(_name) \
	{sizeof(_name) / sizeof(uint32_t), _name, 0}

uint32_t test1()
{
	uint32_t arr1[] = {5, 8, 10, 19};
	uint32_t arr2[] = {1, 4, 9};
	uint32_t arr3[] = {2, 3, 6, 7, 11, 15};

	prox_input_t input[3] = {
		INIT_ARR(arr1),
		INIT_ARR(arr2),
		INIT_ARR(arr3)
	};

	return prox_min_dist(input, 3);
}

int main()
{
	printf("score(minDist=%u) = %f.\n", 1,   prox_calc_score(1));
	printf("score(minDist=%u) = %f.\n", 10,  prox_calc_score(10));
	printf("score(minDist=%u) = %f.\n", 100, prox_calc_score(100));

	printf("=== test1 ===\n");
	printf("res = %u.\n\n", test1());

	mhook_print_unfree();
	return 0;
}
