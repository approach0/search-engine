#include "mhook/mhook.h"
#include "proximity.h"

#define INIT_ARR(_name) \
	{sizeof(_name) / sizeof(uint32_t), _name, 0}

float test1()
{
	uint32_t arr1[] = {3, 5, 6, 19};
	uint32_t arr2[] = {1, 8, 9};
	uint32_t arr3[] = {11, 12, 13, 15, 17};

	prox_input_t input[3] = {
		INIT_ARR(arr1),
		INIT_ARR(arr2),
		INIT_ARR(arr3)
	};

	uint32_t output[128];
	uint32_t n = prox_sort_occurs(output, input, 3);
	printf("sorted:\n");
	for (int i = 0; i < n; i++) {
		printf("%u ", output[i]);
	}
	printf("\n");

	return prox_score(input, 3);
}

int main()
{
	printf("upperbound: %.2f\n", prox_upp());

	printf("=== test1 ===\n");

	printf("res = %f.\n\n", test1());

	mhook_print_unfree();
	return 0;
}
