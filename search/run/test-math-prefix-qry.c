#include <stdio.h>
#include <stdlib.h>
#include "mhook/mhook.h"
#include "math-prefix-qry.h"

#define HIT(qr, ql, dr, dl) \
	ret = pq_hit(&pq, qr, ql, dr, dl); \
	printf("new hit %u-%u, %u-%u ret: %lx\n", qr, ql, dr, dl, ret); \
	pq_print(pq, 8); \
	printf("\n");

#define PRINT_TOP() \
	pq_align(&pq, align_res); \
	for (i = 0; i < 3; i++) { \
		printf("top%u: %u\n", i, align_res[i].width); \
	} \
	printf("\n\n");


int main()
{
	struct math_prefix_qry pq = pq_allocate(16);
	uint32_t i;
	struct pq_align_res align_res[3];
	uint64_t ret;

	HIT(5,6, 4,7);
	HIT(5,6, 4,8);

	HIT(5,7, 4,7);
	HIT(5,7, 4,8);

	HIT(4,8, 3,5);
	HIT(4,8, 3,6);

	HIT(4,9, 3,5);
	HIT(4,9, 3,6);

	PRINT_TOP();

	pq_reset(&pq);

	HIT(1,3, 1,3);
	HIT(1,3, 1,4);
	HIT(1,3, 1,5);
	HIT(1,4, 1,3);
	HIT(1,4, 1,4);
	HIT(1,4, 1,5);

	HIT(2,3, 2,3);
	HIT(2,3, 2,4);
	HIT(2,3, 2,5);
	HIT(2,3, 6,7);
	HIT(2,3, 6,8);

	HIT(2,4, 2,3);
	HIT(2,4, 2,4);
	HIT(2,4, 2,5);
	HIT(2,4, 6,7);
	HIT(2,4, 6,8);

	HIT(5,6, 2,3);
	HIT(5,6, 2,4);
	HIT(5,6, 2,5);
	HIT(5,6, 6,7);
	HIT(5,6, 6,8);

	HIT(5,7, 2,3);
	HIT(5,7, 2,4);
	HIT(5,7, 2,5);
	HIT(5,7, 6,7);
	HIT(5,7, 6,8);

	HIT(5,8, 2,3);
	HIT(5,8, 2,4);
	HIT(5,8, 2,5);
	HIT(5,8, 6,7);
	HIT(5,8, 6,8);

	PRINT_TOP();

	pq_free(pq);
	mhook_print_unfree();
	return 0;
}
