#pragma once
#include <stdint.h>
// #define PQ_CELL_NUMERIC_WEIGHT

struct math_prefix_loc {
	uint32_t qr, dr;
};

struct math_prefix_cell { /* 24 bytes */
	uint64_t qmask;
	uint64_t dmask;
	uint32_t cnt;
	uint16_t qmiss;
	uint16_t dmiss;
};

struct math_prefix_qry {
	uint32_t  n, n_dirty;
	struct math_prefix_loc *dirty;
	struct math_prefix_cell **cell;
};

struct math_prefix_qry pq_allocate(uint32_t);

void pq_print(struct math_prefix_qry, uint32_t);

void pq_free(struct math_prefix_qry);

void pq_reset(struct math_prefix_qry*);

uint64_t pq_hit(struct math_prefix_qry*,
                uint32_t, uint32_t,
                uint32_t, uint32_t);

#ifdef PQ_CELL_NUMERIC_WEIGHT
uint64_t pq_hit_numeric(struct math_prefix_qry*,
                uint32_t, uint32_t, uint32_t,
                uint32_t, uint32_t);
#endif

struct pq_align_res {
	uint32_t width;
	uint32_t qr, dr;
	uint32_t height; /* number of aligned operators */
#ifdef HIGHLIGHT_MATH_ALIGNMENT
	uint64_t qmask;
	uint64_t dmask;
#endif
};

uint32_t
pq_align(struct math_prefix_qry*, struct pq_align_res*, uint32_t*);

void
pq_align_map(struct math_prefix_qry*, uint32_t*, uint32_t*);

void pq_print_dirty_array(struct math_prefix_qry*);

int pq_mask_is_clean(struct math_prefix_qry*);
