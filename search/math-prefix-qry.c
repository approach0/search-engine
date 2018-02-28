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

#define MATH_PREFIX_QRY_JOINT_NODE_CONSTRATINTS
#define MATH_PREFIX_QRY_DEBUG_PRINT
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

void pq_align(struct math_prefix_qry *pq, uint32_t *topk, uint32_t k)
{
	uint32_t i, j;
#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
	uint32_t save_qr, save_dr;
#endif
	uint64_t exclude_qmask = 0;
	uint64_t exclude_dmask = 0;
	struct math_prefix_cell max_cell;
#ifdef MATH_PREFIX_QRY_JOINT_NODE_CONSTRATINTS
	uint64_t cur_max_qmask = 0;
	uint64_t cur_max_dmask = 0;
	uint32_t qmap[MAX_NODE_IDS] = {0};
	uint32_t dmap[MAX_NODE_IDS] = {0};
	uint64_t qmap_mask[MAX_NODE_IDS];
	uint64_t dmap_mask[MAX_NODE_IDS];
#endif

	// for (j = 0; j < pq->n_dirty; j++) {
	// 	struct math_prefix_loc   loc = pq->dirty[j];
	// 	struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];
	// 	printf("%u == %u == %u \n",
	// 		__builtin_popcountll(cell->qmask),
	// 		__builtin_popcountll(cell->dmask),
	// 		cell->cnt
	// 	);
	// }

	for (i = 0; i < k; i++) {
		max_cell.qmask = 0;
		max_cell.dmask = 0;
		max_cell.cnt = 0;
#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
		save_qr = 0;
		save_dr = 0;
#endif

		for (j = 0; j < pq->n_dirty; j++) {
			struct math_prefix_loc   loc = pq->dirty[j];
			struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];

#ifdef MATH_PREFIX_QRY_JOINT_NODE_CONSTRATINTS
			uint64_t overlap_qmask = cur_max_qmask & cell->qmask;
			uint64_t overlap_dmask = cur_max_dmask & cell->dmask;
			if (overlap_qmask && overlap_dmask) {
				if (qmap[loc.qr] == 0 && dmap[loc.dr] == 0) {
#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
					printf("qr%u <-> dr%u \n", loc.qr, loc.dr);
#endif
					qmap[loc.qr] = loc.dr;
					dmap[loc.dr] = loc.qr;
					qmap_mask[loc.qr] = cell->qmask;
					dmap_mask[loc.dr] = cell->dmask;

					printf("qmask[%u] = %lx. \n", loc.qr, cell->qmask);
					printf("dmask[%u] = %lx. \n", loc.dr, cell->dmask);
				} else if (qmap[loc.qr] != loc.dr ||
					       dmap[loc.dr] != loc.qr) {
					int conflict = 0;
					printf("\nconflict qr%u >-< dr%u \n", loc.qr, loc.dr);

					if (dmap[loc.dr]) {
						uint64_t qshadow1 = cell->qmask | qmap_mask[loc.qr];
						uint64_t qshadow2 = qmap_mask[dmap[loc.dr]];
						printf("qshadow %lx & %lx = %lx. \n", qshadow1, qshadow2, qshadow1 & qshadow2);
						if ((qshadow1 & qshadow2) == 0) {
							conflict = 1;
							goto conflict;
						}
					}

					if (qmap[loc.qr]) {
						uint64_t dshadow1 = cell->dmask | dmap_mask[loc.dr]; // 0!!! not c0
						printf("dmap_mask[%u] = %lx. \n", loc.dr, dshadow1);
						uint64_t dshadow2 = dmap_mask[qmap[loc.qr]]; // ff
						printf("dshadow %lx & %lx = %lx. \n", dshadow1, dshadow2, dshadow1 & dshadow2);
						if ((dshadow1 & dshadow2) == 0) {
							conflict = 1;
							goto conflict;
						}
					} 
					//printf("%lx & %lx && %lx & %lx \n", , cell->qmask, dmap_mask[loc.dr], cell->dmask);

conflict:
					if (conflict) {
						topk[i - 1] -= __builtin_popcountll(overlap_qmask);
						cur_max_qmask &= ~ cell->qmask;
						cur_max_dmask &= ~ cell->dmask;
#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
						printf("!!!!! penalty %u =?= %u \n",
							__builtin_popcountll(overlap_qmask),
							__builtin_popcountll(overlap_dmask)
						);
#endif
					}
				}
			}
#endif
			if (exclude_qmask & cell->qmask || exclude_dmask & cell->dmask) {
				continue;
			} else if (cell->cnt > max_cell.cnt) {
				/* greedily assume the two max-subtrees match */
				max_cell.cnt = cell->cnt;
				max_cell.qmask = cell->qmask;
				max_cell.dmask = cell->dmask;
#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
				save_qr = loc.qr;
				save_dr = loc.dr;
#endif
			}
		}

#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
		printf("max_cell[qr%u, dr%u] = %u\n", save_qr, save_dr, max_cell.cnt);
#endif
		topk[i] = max_cell.cnt;
		exclude_qmask |= max_cell.qmask;
		exclude_dmask |= max_cell.dmask;
#ifdef MATH_PREFIX_QRY_JOINT_NODE_CONSTRATINTS
		cur_max_qmask = max_cell.qmask;
		cur_max_dmask = max_cell.dmask;
#endif
	}
}
