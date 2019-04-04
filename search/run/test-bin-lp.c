#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "mhook/mhook.h"

struct bin_lp {
	int n_po, n_nodes;
	int *po, *node; /* row/column map to ID */
	int width, height; /* static constants */
	int *matrix;
};

typedef float (*get_upp_callbk)(int);

struct bin_lp bin_lp_alloc(int height, int width)
{
	struct bin_lp blp;
	blp.n_po = 0;
	blp.n_nodes = 0;
	blp.width = width;
	blp.height = height;
	blp.po   = malloc(sizeof(int) * width);
	blp.node = malloc(sizeof(int) * height);
	blp.matrix = malloc(sizeof(int) * height * width);
	return blp;
}

void bin_lp_print(struct bin_lp blp)
{
	for (int column = 0; column < blp.width; column++)
		if (column < blp.n_po)
			printf(" po-%-3u|", blp.po[column]);
		else
			printf(" po-X  |");
	printf("\n");

	for (int row = 0; row < blp.height; row++) {
		for (int column = 0; column < blp.width; column++)
			printf("%5u   ", blp.matrix[row * blp.width + column]);

		if (row < blp.n_nodes)
			printf(" (node-%d) \n", blp.node[row]);
		else
			printf(" (node-X) \n");
	}
}

int bin_lp_assign(struct bin_lp *blp, int node, int po, int val)
{
	int old_val, modified = 0;
	int column = 0, row = 0;

	for (column = 0; column < blp->n_po; column++)
		if (blp->po[column] == po)
			break;

	if (column == blp->n_po) {
		blp->n_po += 1;
		modified = 2; /* new */
	}

	for (row = 0; row < blp->n_nodes; row++)
		if (blp->node[row] == node)
			break;

	if (row == blp->n_nodes) {
		blp->n_nodes += 1;
		modified = 2; /* new */
	}

	/* assignment */
	blp->po[column] = po;
	blp->node[row] = node;
	old_val = blp->matrix[row * blp->width + column];
	blp->matrix[row * blp->width + column] = val;

	if (!modified && old_val != val)
		modified = 1; /* change value */

	return modified;
}

void bin_lp_reset(struct bin_lp *blp)
{
	blp->n_po = 0;
	blp->n_nodes = 0;
	memset(blp->po, 0, sizeof(int) * blp->width);
	memset(blp->node, 0, sizeof(int) * blp->height);
	memset(blp->matrix, 0, sizeof(int) * blp->height * blp->width);
}

void bin_lp_free(struct bin_lp blp)
{
	free(blp.po);
	free(blp.node);
	free(blp.matrix);
}

/* return pivot of matrix */
int bin_lp_run(float threshold, get_upp_callbk callbk)
{
	int pivot = 0;
	return pivot;
}

int main()
{
	struct bin_lp blp = bin_lp_alloc(3, 5);
	int res = 0;
	bin_lp_reset(&blp);

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
		res |= bin_lp_assign(&blp, 2, 2, 555);
		res |= bin_lp_assign(&blp, 2, 3, 4);
		res |= bin_lp_assign(&blp, 2, 4, 1);

		printf("res = 0x%x\n", res);
		bin_lp_print(blp);
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
		res |= bin_lp_assign(&blp, 2, 2, 5);
		res |= bin_lp_assign(&blp, 2, 3, 4);
		res |= bin_lp_assign(&blp, 2, 4, 1);

		printf("res = 0x%x\n", res);
		bin_lp_print(blp);
	}

	bin_lp_free(blp);

	mhook_print_unfree();
	return 0;
}
