#pragma once
/* in memory inverted list implementation */

#include <stdint.h>
#include "skippy/skippy.h"
#include "codec-buf.h"

/* structures */
struct invlist_node {
	struct skippy_node  sn;
	char               *blk;
	size_t              blk_sz;
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
	 * iterator after invlist being destroyed. */
} *invlist_iter_t;

/* invlist functions */
struct invlist *invlist_create(uint32_t, codec_buf_struct_info_t*);
void invlist_free(struct invlist*);

/* iterator functions */
size_t invlist_iter_write(struct invlist_iterator*, const void*);
size_t invlist_iter_flush(struct invlist_iterator*);

void invlist_iter_free(struct invlist_iterator*);

///* postlist_iterator functions */
//postlist_iter_t postlist_iterator(struct postlist*);
//int             postlist_empty(struct postlist*);
//int             postlist_iter_next(struct postlist_iterator*);
//uint64_t        postlist_iter_cur(struct postlist_iterator*);
//void*           postlist_iter_cur_item(struct postlist_iterator*);
//int             postlist_iter_jump(struct postlist_iterator*, uint64_t);
//int             postlist_iter_jump32(struct postlist_iterator*, uint32_t);
