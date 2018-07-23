#pragma once
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "rank.h" /* for hit_occur_t */

typedef struct {
	uint32_t     n_pos; /* number of positions */
	hit_occur_t *pos; /* position array */
	uint32_t     cur; /* current index of position array */
} prox_input_t;

uint32_t prox_min_dist(prox_input_t*, uint32_t);

float prox_calc_score(uint32_t);

static __inline void
prox_set_input(prox_input_t *in, hit_occur_t* pos, uint32_t n_pos)
{
	in->n_pos = n_pos;
	in->pos = pos;
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

void prox_print(prox_input_t*, uint32_t);
