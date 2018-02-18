#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "math-prefix-qry.h"

struct math_prefix_qry
pq_allocate(uint32_t max_id, uint32_t k)
{
	struct math_prefix_qry pq;
	uint32_t i;

	pq.n = max_id + 1;
	pq.k = k;
	pq.n_dirty = 0;

	pq.vol = calloc(pq.n, sizeof(uint32_t));
	pq.topcnt = calloc(pq.k + 1, sizeof(uint32_t));
	pq.toploc = calloc(pq.k + 1, sizeof(struct math_prefix_loc));
	pq.dirty = calloc(pq.n * MAX_NODE_IDS, sizeof(struct math_prefix_loc));

	pq.cnt = calloc(pq.n, sizeof(uint32_t *));
	for (i = 0; i < pq.n; i++)
		pq.cnt[i] = calloc(MAX_NODE_IDS, sizeof(uint32_t));

	return pq;
}

void pq_free(struct math_prefix_qry pq)
{
	for (uint32_t i = 0; i < pq.n; i++)
		free(pq.cnt[i]);
	free(pq.cnt);

	free(pq.vol);
	free(pq.topcnt);
	free(pq.toploc);
	free(pq.dirty);
}

void pq_reset(struct math_prefix_qry *pq)
{
	for (uint32_t i = 0; i < pq->n_dirty; i++) {
		struct math_prefix_loc loc = pq->dirty[i];
		pq->cnt[loc.qr][loc.dr] = 0;
	}

	pq->n_dirty = 0;
	memset(pq->topcnt, 0, pq->k * sizeof(uint32_t));
}

void
pq_hit(struct math_prefix_qry *pq, uint32_t qr, uint32_t dr)
{
	uint32_t cnt, i, k = pq->k;
	if (pq->vol[qr] <= pq->topcnt[k - 1])
		return;

	cnt = (++ pq->cnt[qr][dr]);

	for (i = k - 1; cnt > pq->topcnt[i]; i--) {
		/* swap */
		pq->topcnt[i + 1] = pq->topcnt[i];
		pq->toploc[i + 1] = pq->toploc[i];

		pq->topcnt[i] = cnt;
		pq->toploc[i].qr = qr;
		pq->toploc[i].dr = dr;

		if (i == 0)
			break;
	}
}
