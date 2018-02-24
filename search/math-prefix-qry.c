#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "math-prefix-qry.h"

struct math_prefix_qry
pq_allocate(uint32_t q_maxid)
{
	struct math_prefix_qry pq;
	uint32_t i;

	pq.n = q_maxid + 1;
	pq.n_dirty = 0;

	pq.dirty = calloc(pq.n * MAX_NODE_IDS, sizeof(struct math_prefix_loc));

	pq.cell = calloc(pq.n, sizeof(struct math_prefix_cell *));
	for (i = 0; i < pq.n; i++)
		pq.cell[i] = calloc(MAX_NODE_IDS, sizeof(struct math_prefix_cell));

 	return pq;
}

void pq_free(struct math_prefix_qry pq)
{
	for (uint32_t i = 0; i < pq.n; i++)
		free(pq.cell[i]);
	free(pq.cell);

	free(pq.dirty);
}

void pq_reset(struct math_prefix_qry *pq)
{
	for (uint32_t i = 0; i < pq->n_dirty; i++) {
		struct math_prefix_loc loc = pq->dirty[i];
		memset(&pq->cell[loc.qr][loc.dr], 0, sizeof(struct math_prefix_cell));
	}

	pq->n_dirty = 0;
}

#define C_GRAY    "\033[1m\033[30m"
#define C_RST     "\033[0m"

void pq_print(struct math_prefix_qry pq, uint32_t d_maxid)
{
	uint32_t i, j, k;
	//printf("n_dirty = %u\n", pq.n_dirty);

	printf("%3s | ", "qr");
	for (j = 0; j < d_maxid; j++) {
		printf("[%2u] ", j);
	}
	printf("\n");

	for (i = 0; i < pq.n; i++) {
		printf("%3u | ", i);
		for (j = 0; j < d_maxid; j++) {
			struct math_prefix_loc loc = {i, j};
			for (k = 0; k < pq.n_dirty; k++) {
				if (0 == memcmp(&loc, pq.dirty + k,
				                sizeof(struct math_prefix_loc)))
					break;
			}

			if (k == pq.n_dirty)
				printf(C_GRAY);
			printf(" %2u  ", pq.cell[i][j].cnt);
			printf(C_RST);
		}
		printf("\n");
	}
}

uint64_t pq_hit(struct math_prefix_qry *pq,
                uint32_t qr, uint32_t ql,
                uint32_t dr, uint32_t dl)
{
	struct math_prefix_cell *cell;
	uint64_t ql_bit, dl_bit;

	if (dr >= MAX_NODE_IDS)
		return dr;

	cell = pq->cell[qr] + dr;
	if (cell->cnt == 0) {
		pq->dirty[pq->n_dirty].qr = qr;
		pq->dirty[pq->n_dirty].dr = dr;
		pq->n_dirty++;
	}

	ql_bit = 1ULL << (ql - 1);
	dl_bit = 1ULL << (dl - 1);

	if (ql_bit & cell->qmask) {
		cell->dmiss ++;
		return cell->qmask;
	} else if (dl_bit & cell->dmask) {
		cell->qmiss ++;
		return cell->dmask;
	} else {
		cell->cnt += 1;
		cell->qmask |= ql_bit;
		cell->dmask |= dl_bit;
		return 0;
	}
}

void pq_align(struct math_prefix_qry *pq, uint32_t *topk, uint32_t k)
{
	uint64_t qmask = 0;
	uint64_t dmask = 0;
	uint32_t i, j;
	struct math_prefix_cell max_cell;

	for (i = 0; i < k; i++) {
		max_cell.qmask = 0;
		max_cell.dmask = 0;
		max_cell.cnt = 0;
		for (j = 0; j < pq->n_dirty; j++) {
			struct math_prefix_loc   loc = pq->dirty[j];
			struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];
			uint64_t qr_subset = qmask & cell->qmask;
			uint64_t dr_subset = dmask & cell->dmask;
			if (qr_subset & dr_subset) {
				printf("qr%u <-> dr%u \n", loc.qr, loc.dr);
				// topk[i - 1] = 0;
				continue;
			} else if (qr_subset | dr_subset) {
				//printf("qr%u >-< dr%u \n", loc.qr, loc.dr);
				continue;
			} else if (cell->cnt > max_cell.cnt) {
				max_cell.cnt = cell->cnt;
				max_cell.qmask = cell->qmask;
				max_cell.dmask = cell->dmask;
			}
		}

		topk[i] = max_cell.cnt;
		qmask |= max_cell.qmask;
		dmask |= max_cell.dmask;
	}
}
