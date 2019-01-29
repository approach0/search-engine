#include "mhook/mhook.h"
#include "config.h"
#include "term-index/term-index.h"
#include "proximity.h"

#define INIT_ARR(_name) \
	{sizeof(_name) / sizeof(hit_occur_t), _name, 0}

uint32_t test1()
{
#ifdef HIGHLIGHT_MATH_ALIGNMENT
	hit_occur_t arr1[] = {{5,{0},{0}}, {8,{0},{0}}, {10,{0},{0}}, {19,{0},{0}}};
	hit_occur_t arr2[] = {{1,{0},{0}}, {4,{0},{0}}, {9,{0},{0}}};
	hit_occur_t arr3[] = {{2,{0},{0}}, {3,{0},{0}}, {6,{0},{0}}, {7,{0},{0}},
	                      {11,{0},{0}}, {15,{0},{0}}};
#else
	hit_occur_t arr1[] = {{5}, {8}, {10}, {19}};
	hit_occur_t arr2[] = {{1}, {4}, {9}};
	hit_occur_t arr3[] = {{2}, {3}, {6}, {7}, {11}, {15}};
#endif

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
