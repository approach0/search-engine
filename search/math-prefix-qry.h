#include <stdint.h>

#define MAX_NODE_IDS 4096

struct math_prefix_loc {
	uint32_t qr, dr;
};

struct math_prefix_qry {
	uint32_t  n, k, n_dirty, *vol, *topcnt;
	struct math_prefix_loc *toploc, *dirty;
	uint32_t **cnt;
};

struct math_prefix_qry pq_allocate(uint32_t, uint32_t);

void pq_free(struct math_prefix_qry);

void pq_reset(struct math_prefix_qry*);
