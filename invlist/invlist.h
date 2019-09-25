#pragma once
/* in memory inverted list implementation */

#include <stdint.h>
#include "skippy/skippy.h"
#include "codec-buf.h"

/* structures */
struct invlist_node {
	struct skippy_node  sn;
	char               *blk;
	unsigned int        len;
};

struct invlist {
	struct invlist_node     *head;
	size_t                   tot_sz;
	uint32_t                 n_blk;
	struct skippy            skippy;
	uint32_t                 buf_max_len;
	uint32_t                 buf_max_sz;
	codec_buf_struct_info_t *c_info;
};

typedef struct invlist_iterator {
	struct invlist_node     *cur;
	void                   **buf;
	uint32_t                 buf_idx;
	uint32_t                 buf_len;
	struct invlist          *invlist;

	codec_buf_struct_info_t *c_info;
	/* we need c_info here so that we can free
	 * iterator buf even after invlist gets destroyed. */
} *invlist_iter_t;

/* invlist functions */
struct invlist *invlist_create(uint32_t, codec_buf_struct_info_t*);
void invlist_free(struct invlist*);

/* iterator functions */
invlist_iter_t invlist_writer(struct invlist*);
invlist_iter_t invlist_iterator(struct invlist*);

void invlist_iter_free(struct invlist_iterator*);

size_t invlist_iter_flush(struct invlist_iterator*);
size_t invlist_iter_write(struct invlist_iterator*, const void*);

int invlist_empty(struct invlist*);
int invlist_iter_next(struct invlist_iterator*);
uint64_t invlist_iter_curkey(struct invlist_iterator*);
size_t invlist_iter_read(struct invlist_iterator*, void*);

void invlist_print_as_decoded_ints(struct invlist*);

//int postlist_iter_jump(struct postlist_iterator*, uint64_t);
//int postlist_iter_jump32(struct postlist_iterator*, uint32_t);
