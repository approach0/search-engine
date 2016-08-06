#pragma once
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

typedef struct {
	uint32_t    n_pos; /* number of positions */
	position_t *pos_arr; /* position array */
	uint32_t    cur; /* current index of position array */
} prox_input_t;

position_t prox_min_dist(prox_input_t*, uint32_t);

float prox_calc_score(position_t);

static __inline void
prox_set_input(prox_input_t *in, position_t* arr, uint32_t n_pos)
{
	in->n_pos = n_pos;
	in->pos_arr = arr;
	in->cur = 0;
}

static __inline void
prox_reset_inputs(prox_input_t *in, uint32_t n)
{
	uint32_t i;
	for (i = 0; i < n; i++)
		in[i].cur = 0;
}

#define MAX_N_POSITIONS UINT_MAX
