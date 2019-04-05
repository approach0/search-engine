#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bin-lp.h"

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

void bin_lp_print_all(struct bin_lp blp)
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

void bin_lp_print(struct bin_lp blp)
{
	for (int column = 0; column < blp.n_po; column++)
		printf(" po-%-3u|", blp.po[column]);
	printf("\n");

	for (int row = 0; row < blp.n_nodes; row++) {
		for (int column = 0; column < blp.n_po; column++)
			printf("%5u   ", blp.matrix[row * blp.width + column]);
		printf(" (node-%d) \n", blp.node[row]);
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

static int column_max(struct bin_lp *blp, int c)
{
	int max = 0;
	for (int row = 0; row < blp->n_nodes; row++)
		if (blp->matrix[row * blp->width + c] > max)
			max = blp->matrix[row * blp->width + c];
	return max;
}

static void column_swap(struct bin_lp *blp, int a, int b)
{
	int tmp;
	for (int row = 0; row < blp->n_nodes; row++) {
		tmp = blp->matrix[row * blp->width + a];
		blp->matrix[row * blp->width + a] = blp->matrix[row * blp->width + b];
		blp->matrix[row * blp->width + b] = tmp;
	}

	tmp = blp->po[a];
	blp->po[a] = blp->po[b];
	blp->po[b] = tmp;
}

static int row_sum(struct bin_lp *blp, int r, int pivot)
{
	int sum = 0;
	for (int column = pivot; column < blp->n_po; column++)
		sum += blp->matrix[r * blp->width + column];
	return sum;
}

static int row_max_column(struct bin_lp *blp, int r, int pivot)
{
	int max_column = 0, max = 0;
	for (int column = pivot; column < blp->n_po; column++)
		if (blp->matrix[r * blp->width + column] > max) {
			max = blp->matrix[r * blp->width + column];
			max_column = column;
		}
	return max_column;
}

/* return pivot of matrix */
int
bin_lp_run(struct bin_lp *blp, float threshold, get_upp_callbk upp, void *args)
{
	int pivot = 0;
	int column, row;

#ifdef DEBUG_BIN_LP
	printf("INPUT: (pivot = %d)\n", pivot);
	bin_lp_print(*blp);
#endif

	/* initialize */
	for (column = 0; column < blp->n_po; column++) {
		int max = column_max(blp, column);

		/* violates constraints anyway, take out */
		if (upp(max, args) > threshold) {
			column_swap(blp, pivot ++, column);
		}
	}

	/* pick out all required posting lists that can make threshold value */
	while (pivot < blp->n_po) {
#ifdef DEBUG_BIN_LP
		printf("pivot = %d\n", pivot);
		bin_lp_print(*blp);
#endif
		/* find out which row violates threshold constraints the most */
		int max_row = 0, max = 0;
		for (row = 0; row < blp->n_nodes; row++) {
			int sum = row_sum(blp, row, pivot);
			if (sum > max) {
				max = sum;
				max_row = row;
			}
		}
#ifdef DEBUG_BIN_LP
		printf("max_row = %d, val = %d\n", max_row, max);
#endif
		if (upp(max, args) > threshold) {
			/* find out the max element in the violating row */
			int max_col = row_max_column(blp, max_row, pivot);
			column_swap(blp, pivot ++, max_col);
		} else {
			break;
		}
	}

	return pivot;
}
