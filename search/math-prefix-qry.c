#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
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

/* for debug */
int pq_mask_is_clean(struct math_prefix_qry *pq)
{
	uint64_t res = 0;
	for (uint32_t i = 0; i < pq->n; i++) {
		for (uint32_t j = 0; j < MAX_NODE_IDS; j++) {
			res |= pq->cell[i][j].qmask;
			res |= pq->cell[i][j].dmask;
			if (res != 0) {
				printf("cell[%u][%u] mask is not clear\n", i, j);
				return 0;
			}
		}
	}
	return 1;
}

void pq_reset(struct math_prefix_qry *pq)
{
	for (uint32_t i = 0; i < pq->n_dirty; i++) {
		struct math_prefix_loc loc = pq->dirty[i];
		memset(&pq->cell[loc.qr][loc.dr], 0, sizeof(struct math_prefix_cell));
	}

	pq->n_dirty = 0;
	// assert(pq_mask_is_clean(pq))
}

void pq_print_dirty_array(struct math_prefix_qry *pq)
{
	uint32_t i;
	printf("dirty array (len = %u): ", pq->n_dirty);
	for (i = 0; i < pq->n_dirty; i++) {
		struct math_prefix_loc   loc = pq->dirty[i];
		struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];
		printf("[%u @ (%u, %u)]", cell->cnt, loc.qr, loc.dr);
		// printf("%u == %u == %u \n",
		// 	__builtin_popcountll(cell->qmask),
		// 	__builtin_popcountll(cell->dmask),
		// 	cell->cnt
		// );
	}
	printf("\n");
}

#define C_GRAY    "\033[1m\033[30m"
#define C_RST     "\033[0m"

void pq_print(struct math_prefix_qry pq, uint32_t d_maxid)
{
	uint32_t i, j, k;
	printf("n_dirty = %u\n", pq.n_dirty);
	pq_print_dirty_array(&pq);

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

#ifdef PQ_CELL_NUMERIC_WEIGHT
uint64_t pq_hit_numeric(struct math_prefix_qry *pq,
	uint32_t qr, uint32_t ql, uint32_t dr, uint32_t dl,
	uint32_t w)
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
		cell->cnt += w;
		cell->qmask |= ql_bit;
		cell->dmask |= dl_bit;
		return 0;
	}
}
#endif

uint32_t
pq_align(struct math_prefix_qry *pq,
         struct pq_align_res *res, uint32_t *visibimap)
{
	uint64_t exclude_qmask = 0;
	uint64_t exclude_dmask = 0;
	uint32_t r_cnt = 0;

#ifdef MATH_COMPUTE_R_CNT
	uint64_t matched_qmask[MAX_MTREE] = {0};
	uint64_t matched_dmask[MAX_MTREE] = {0};
	uint32_t qmap[MAX_NODE_IDS] = {0};
	uint32_t dmap[MAX_NODE_IDS] = {0};
#endif

	/* find the top-K disjoint cell (i.e. common subtrees) */
	for (uint32_t j = 0; j < MAX_MTREE; j++) {
		uint32_t max = 0;
		uint32_t max_qr = 0, max_dr = 0;
		uint64_t max_qmask, max_dmask;

		/* for all the dirty cells, find the maximum disjoint cell */
		for (uint32_t i = 0; i < pq->n_dirty; i++) {
			struct math_prefix_loc loc = pq->dirty[i];
			struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];

			if (exclude_qmask & cell->qmask || exclude_dmask & cell->dmask) {
				/* intersect */
				continue;
			} else if (cell->cnt > max) {
				/* disjoint */
				max = cell->cnt;
				max_qr = loc.qr;
				max_dr = loc.dr;
				max_qmask = cell->qmask;
				max_dmask = cell->dmask;
			}
		}

		if (max) {
			/* found the next maximum disjoint cell */
			res[j].width = max;
			res[j].qr = max_qr;
			res[j].dr = max_dr;
#ifdef MATH_SLOW_SEARCH
			res[j].qmask = max_qmask;
			res[j].dmask = max_dmask;
#endif

			exclude_qmask |= max_qmask;
			exclude_dmask |= max_dmask;

#ifdef MATH_COMPUTE_R_CNT
			matched_qmask[j] = max_qmask;
			matched_dmask[j] = max_dmask;
#endif
		} else {
			/* exhuasted all possibility */
			break;
		}
	}

#ifdef MATH_COMPUTE_R_CNT
	/* go through all the dirty cells again, find internal nodes mapping. */
	for (uint32_t i = 0; i < pq->n_dirty; i++) {
		struct math_prefix_loc loc = pq->dirty[i];
		struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];

		for (uint32_t j = 0; j < MAX_MTREE; j++) {
			uint64_t overlap_qmask = matched_qmask[j] & cell->qmask;
			uint64_t overlap_dmask = matched_dmask[j] & cell->dmask;

			if (overlap_qmask && overlap_dmask) {
				/* internal pair of nodes */
				if (qmap[loc.qr] == 0 && dmap[loc.dr] == 0) {
					qmap[loc.qr] = loc.dr;
					dmap[loc.dr] = loc.qr;

#ifdef CNT_VISIBLE_NODES_ONLY
					if (visibimap && visibimap[loc.qr])
#endif
					{
						r_cnt ++;
						res[j].height += 1;
#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
						printf("qr%u <-> dr%u \n", loc.qr, loc.dr);
#endif
					}
				}
#ifdef MATH_PREFIX_QRY_DEBUG_PRINT
				else if (qmap[loc.qr] != loc.dr ||
						 dmap[loc.dr] != loc.qr) {
					printf("qr%u >-< dr%u \n", loc.qr, loc.dr);
				}
#endif
				break;
			}
		}
	}
#endif

	return r_cnt;
}

void
pq_align_map(struct math_prefix_qry *pq, uint32_t *qmap, uint32_t *dmap)
{
	uint64_t exclude_qmask = 0;
	uint64_t exclude_dmask = 0;

	uint64_t matched_qmask[MAX_MTREE] = {0};
	uint64_t matched_dmask[MAX_MTREE] = {0};

	/* find the top-K disjoint cell (i.e. common subtrees) */
	for (uint32_t j = 0; j < MAX_MTREE; j++) {
		uint32_t max = 0;
		uint64_t max_qmask = 0, max_dmask = 0;

		/* for all the dirty cells, find the maximum disjoint cell */
		for (uint32_t i = 0; i < pq->n_dirty; i++) {
			struct math_prefix_loc loc = pq->dirty[i];
			struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];

			if (exclude_qmask & cell->qmask || exclude_dmask & cell->dmask) {
				/* intersect */
				continue;
			} else if (cell->cnt > max) {
				/* disjoint */
				max = cell->cnt;
				max_qmask = cell->qmask;
				max_dmask = cell->dmask;
			}
		}

		if (max) {
			exclude_qmask |= max_qmask;
			exclude_dmask |= max_dmask;

			matched_qmask[j] = max_qmask;
			matched_dmask[j] = max_dmask;
		} else {
			/* exhuasted all possibility */
			break;
		}
	}

	/* go through all the dirty cells again, find internal nodes mapping. */
	for (uint32_t i = 0; i < pq->n_dirty; i++) {
		struct math_prefix_loc loc = pq->dirty[i];
		struct math_prefix_cell *cell = &pq->cell[loc.qr][loc.dr];

		for (uint32_t j = 0; j < MAX_MTREE; j++) {
			uint64_t overlap_qmask = matched_qmask[j] & cell->qmask;
			uint64_t overlap_dmask = matched_dmask[j] & cell->dmask;

			if (overlap_qmask && overlap_dmask) {
				/* internal pair of nodes */
				if (qmap[loc.qr] == 0 && dmap[loc.dr] == 0) {
					qmap[loc.qr] = j + 1; /* assign matched subtree ID */
					dmap[loc.dr] = j + 1; /* assign matched subtree ID */
				}
				break;
			}
		}
	}

	/* go through matched leaves, find leaves mapping. */
	for (uint32_t j = 0; j < MAX_MTREE; j++) {
		for (uint32_t bit = 0; bit < MAX_LEAVES; bit++) {
			if ((0x1L << bit) & matched_qmask[j])
				qmap[bit + 1] = j + 1;
			if ((0x1L << bit) & matched_dmask[j])
				dmap[bit + 1] = j + 1;
		}
	}
}
