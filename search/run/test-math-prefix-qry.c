#include <stdio.h>
#include <stdlib.h>
#include "mhook/mhook.h"
#include "math-prefix-qry.h"

int main()
{
	uint32_t i, j;
	struct math_prefix_qry pq = pq_allocate(16, 3);

	for (i = 0; i < 6543210; i++) {
		pq_reset(&pq);
		pq.n_dirty = 63;
		for (j = 0; j < pq.n_dirty; j++) {
			struct math_prefix_loc loc = {i % 16, j};
			pq.dirty[j] = loc;
		}
	}

	pq_free(pq);
	mhook_print_unfree();
	return 0;
}
