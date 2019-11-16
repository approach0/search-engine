#include <stdlib.h>
#include "config.h" /* for MAX_TOTAL_OCCURS */
#include "proximity.h"

void prox_print(prox_input_t *in, uint32_t n)
{
	uint32_t i, j;
	for (i = 0; i < n; i++) {
		printf("position_array[%d] (len=%u): ", i, in[i].n_pos);
		for (j = 0; j < in[i].n_pos; j++)
			if (j == in[i].cur)
				printf("[%d] ", in[i].pos[j]);
			else
				printf("%d ", in[i].pos[j]);
		printf("\n");
	}
}

#define CUR_POS(_in, _i) \
	_in[_i].pos[_in[_i].cur]

uint32_t prox_min_dist(prox_input_t* in, uint32_t n)
{
	uint32_t last_idx, last = UINT_MAX;
	uint32_t min_dist = UINT_MAX;

	while (1) {
		uint32_t i, min_idx, min = UINT_MAX;

		for (i = 0; i < n; i++)
			if (in[i].cur < in[i].n_pos)
				if (CUR_POS(in, i) < min) {
					min = CUR_POS(in, i);
					min_idx = i;
				}

#ifdef DEBUG_PROXIMITY
		prox_print(in, n);
		printf("last: %u from [%u].\n", last, last_idx);
		printf("min: %u from [%u].\n", min, min_idx);
#endif

		if (min == UINT_MAX)
			break;
		else if (last != UINT_MAX)
			/* after the first position is iterated */
			if (min != last && /* min_dist != 0 */
			    min_idx != last_idx &&
			    min - last < min_dist) {

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

#ifdef DEBUG_PROXIMITY
	printf("final min_dist = %u\n", min_dist);
#endif
	return min_dist;
}

uint32_t
prox_sort_occurs(uint32_t *dest, prox_input_t* in, int n)
{
	uint32_t dest_end = 0;
	while (dest_end < MAX_TOTAL_OCCURS) {
		uint32_t i, min_idx, min_cur, min = UINT_MAX;

		for (i = 0; i < n; i++)
			if (in[i].cur < in[i].n_pos)
				if (CUR_POS(in, i) < min) {
					min = CUR_POS(in, i);
					min_idx = i;
					min_cur = in[i].cur;
				}

		if (min == UINT_MAX)
			/* input exhausted */
			break;
		else
			/* consume input */
			in[min_idx].cur ++;

		if (dest_end == 0 || /* first put */
		    dest[dest_end - 1] != min /* unique */)
			/* copy entire element */
			dest[dest_end++] = in[min_idx].pos[min_cur];
	}
	return dest_end;
}

#include <math.h>

float prox_calc_score(uint32_t min_dist)
{
	float dis = (float)min_dist;

	return logf(0.3f + expf(-dis));
}

float proximity_score(prox_input_t *prox, int n)
{
	uint32_t min_dist = prox_min_dist(prox, n);
	return prox_calc_score(min_dist);
}
