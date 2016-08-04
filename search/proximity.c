#include <stdlib.h>
#include "term-index/term-index.h" /* for position_t */
#include "proximity.h"
#include "config.h"

static void debug_print(prox_input_t *in, uint32_t n)
{
	uint32_t i, j;
	for (i = 0; i < n; i++) {
		printf("position_array[%d] (len=%u): ", i, in[i].n_pos);
		for (j = 0; j < in[i].n_pos; j++)
			if (j == in[i].cur)
				printf("[%d] ", in[i].pos_arr[j]);
			else
				printf("%d ", in[i].pos_arr[j]);
		printf("\n");
	}
}

#define CUR_POS(_in, _i) \
	_in[_i].pos_arr[_in[_i].cur]

position_t prox_min_dist(prox_input_t* in, uint32_t n)
{
	uint32_t last_idx, last = MAX_N_POSITIONS;
	uint32_t min_dist = MAX_N_POSITIONS;

	while (1) {
		uint32_t i, min_idx, min = MAX_N_POSITIONS;

		for (i = 0; i < n; i++)
			if (in[i].cur < in[i].n_pos)
				if (CUR_POS(in, i) < min) {
					min = CUR_POS(in, i);
					min_idx = i;
				}

#ifdef DEBUG_PROXIMITY
		debug_print(in, n);
		printf("last: %u from [%u].\n", last, last_idx);
		printf("min: %u from [%u].\n", min, min_idx);
#endif

		if (min == MAX_N_POSITIONS)
			break;
		else if (last != MAX_N_POSITIONS)
			/* after the first position is iterated */
			if (min_idx != last_idx && min - last < min_dist) {
				min_dist = min - last;
#ifdef DEBUG_PROXIMITY
				printf("minDist updated: %u.\n", min_dist);
#endif
			}

		in[min_idx].cur ++;
		last = min;
		last_idx = min_idx;

#ifdef DEBUG_PROXIMITY
		printf("\n");
#endif
	}

	return min_dist;
}

#include <math.h>

float prox_calc_score(position_t min_dist)
{
	float dis = (float)min_dist;

	return logf(0.3f + expf(-dis));
}
