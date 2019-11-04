#pragma once

struct bin_lp {
	int n_po, n_nodes; /* dynamical lengths */
	int *po, *node; /* row/column map to ID */
	int width, height; /* static constants */
	int *matrix, *weight; /* coefficients */
	int zero_pivot, one_pivot; /* pivots */
};

typedef float (*bin_lp_upp_callbk)(void*, float);

struct bin_lp bin_lp_alloc(int, int);
void   bin_lp_print(struct bin_lp, int);
int    bin_lp_assign(struct bin_lp*, int, int, int);
void   bin_lp_reset(struct bin_lp*);
void   bin_lp_free(struct bin_lp);
int    bin_lp_solve(struct bin_lp*, float, bin_lp_upp_callbk, void*);
