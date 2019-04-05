#pragma once

struct bin_lp {
	int n_po, n_nodes;
	int *po, *node; /* row/column map to ID */
	int width, height; /* static constants */
	int *matrix;
};

typedef float (*get_upp_callbk)(int);

struct bin_lp bin_lp_alloc(int, int);
void   bin_lp_print(struct bin_lp);
int    bin_lp_assign(struct bin_lp*, int, int, int);
void   bin_lp_reset(struct bin_lp*);
void   bin_lp_free(struct bin_lp);
int    bin_lp_run(struct bin_lp*, float, get_upp_callbk);
