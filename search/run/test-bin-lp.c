#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mhook/mhook.h"
#include "bin-lp.h"

static float test_get_upperbound(int i, void *_)
{
	return (float)i;
}


static int test_set_weight(int i, void *_)
{
	return 2;
}

int main()
{
	struct bin_lp blp = bin_lp_alloc(3, 5);
	int res = 0;
	bin_lp_reset(&blp);

	{
		/* second row */
		res |= bin_lp_assign(&blp, 1, 1, 3);
		res |= bin_lp_assign(&blp, 1, 2, 2);
		res |= bin_lp_assign(&blp, 1, 3, 1);

		/* first row */
		res |= bin_lp_assign(&blp, 0, 0, 4);
		res |= bin_lp_assign(&blp, 0, 1, 2);
		res |= bin_lp_assign(&blp, 0, 2, 3);
		res |= bin_lp_assign(&blp, 0, 3, 1);

		/* third row */
		res |= bin_lp_assign(&blp, 2, 2, 555);
		res |= bin_lp_assign(&blp, 2, 3, 4);
		res |= bin_lp_assign(&blp, 2, 4, 1);

		printf("res = 0x%x\n", res);
	}

	res = 0;
	{
		/* first row */
		res |= bin_lp_assign(&blp, 0, 0, 4);
		res |= bin_lp_assign(&blp, 0, 1, 2);
		res |= bin_lp_assign(&blp, 0, 2, 3);
		res |= bin_lp_assign(&blp, 0, 3, 1);

		/* second row */
		res |= bin_lp_assign(&blp, 1, 1, 3);
		res |= bin_lp_assign(&blp, 1, 2, 2);
		res |= bin_lp_assign(&blp, 1, 3, 1);

		/* third row */
		res |= bin_lp_assign(&blp, 2, 2, 6);
		res |= bin_lp_assign(&blp, 2, 3, 4);
		res |= bin_lp_assign(&blp, 2, 4, 1);

		printf("res = 0x%x\n", res);
	}

	res = bin_lp_solve(&blp, 5.f, &test_get_upperbound, NULL);
	for (int i = 0; i < res; i++)
		printf("po[%d] is of requirement set.\n", blp.po[i]);

	bin_lp_free(blp);

	mhook_print_unfree();
	return 0;
}
