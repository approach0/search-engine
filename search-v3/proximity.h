#pragma once
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

typedef struct {
	uint32_t  n_pos; /* number of positions */
	uint32_t *pos; /* position array */
	uint32_t  cur; /* current index of position array */
} prox_input_t;

static __inline void
prox_set_input(prox_input_t *in, uint32_t* pos, uint32_t n_pos)
{
	in->n_pos = n_pos;
	in->pos = pos;
	in->cur = 0;
}

float    prox_upp(); /* proximity score upperbound */
float    prox_score(prox_input_t*, int);
uint32_t prox_sort_occurs(uint32_t*, prox_input_t*, int);

void prox_print(prox_input_t*, uint32_t);
