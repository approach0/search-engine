#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mhook/mhook.h"
#include "bin-lp.h"

static float test_get_upperbound(void *_, float val)
{
	return val;
}

int main()
{
	struct bin_lp blp = bin_lp_alloc(3, 5);
	int i, pivot, res = 0;
	bin_lp_reset(&blp);

	{
		/* second row */
		res |= bin_lp_assign(&blp, 1, 1, 3.f);
		res |= bin_lp_assign(&blp, 1, 2, 2.f);
		res |= bin_lp_assign(&blp, 1, 3, 1.f);

		/* first row */
		res |= bin_lp_assign(&blp, 0, 0, 4.f);
		res |= bin_lp_assign(&blp, 0, 1, 2.f);
		res |= bin_lp_assign(&blp, 0, 2, 3.f);
		res |= bin_lp_assign(&blp, 0, 3, 1.f);

		/* third row */
		res |= bin_lp_assign(&blp, 2, 2, 555.f);
		res |= bin_lp_assign(&blp, 2, 3, 4.f);
		res |= bin_lp_assign(&blp, 2, 4, 1.f);

		printf("res = 0x%x\n", res);
	}

	res = 0;
	{
		/* first row */
		res |= bin_lp_assign(&blp, 0, 0, 4.f);
		res |= bin_lp_assign(&blp, 0, 1, 2.f);
		res |= bin_lp_assign(&blp, 0, 2, 3.f);
		res |= bin_lp_assign(&blp, 0, 3, 1.f);

		/* second row */
		res |= bin_lp_assign(&blp, 1, 1, 3.f);
		res |= bin_lp_assign(&blp, 1, 2, 2.f);
		res |= bin_lp_assign(&blp, 1, 3, 1.f);

		/* third row */
		res |= bin_lp_assign(&blp, 2, 2, 6.f);
		res |= bin_lp_assign(&blp, 2, 3, 4.f);
		res |= bin_lp_assign(&blp, 2, 4, 1.f);

		printf("res = 0x%x\n", res);
	}

	/* set objective weights */
	for (i = 0; i < blp.n_po; i++) {
		blp.weight[i] = 1.f;
	}
	blp.weight[4] = 3.f;

	/* solve binary programming problem */
	pivot = bin_lp_solve(&blp, 5.f, &test_get_upperbound, NULL);

	/* print results */
	for (i = 0; i < pivot; i++)
		printf("po[%d] is in requirement set.\n", blp.po[i]);
	for (i = pivot; i < blp.n_po; i++)
		printf("po[%d] is in skipping set.\n", blp.po[i]);

	bin_lp_free(blp);

	mhook_print_unfree();
	return 0;
}
