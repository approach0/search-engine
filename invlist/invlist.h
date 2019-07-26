#pragma once
#include <stdint.h>
#include "skippy/skippy.h"
#include "codec-buf.h"

/* callback types */
typedef uint64_t (*invlist_flush_callbk)(char*, uint32_t*, void *);
typedef void     (*invlist_rebuf_callbk)(char*, uint32_t*, void *);
typedef void     (*invlist_free_callbk)(void *);

struct invlist_callbks {
	invlist_flush_callbk on_flush;
	invlist_rebuf_callbk on_rebuf;
	invlist_free_callbk  on_free;
};

/* structures */
struct invlist_node {
	struct skippy_node  sn;
	char               *blk;
	size_t              blk_sz;
};

typedef struct invlist_node_codec_buf {
	struct invlist_node     *cur;
	void                   **buf;
	uint32_t                 buf_idx;
	uint32_t                 buf_end; /* in bytes */
	codec_buf_struct_info_t *struct_info; /* codec info */
} invlist_node_codec_buf_t;

struct invlist {
	/* skippy book keeping */
	struct invlist_node     *head, *tail;
	size_t                   tot_sz; /* size stats */
	uint32_t                 n_blk;
	struct skippy            skippy;
	uint32_t                 buf_sz; /* in bytes */

	/* callback functions */
	struct invlist_callbks   calls;

	/* write buffer/iterator */
	invlist_node_codec_buf_t wr_buf;
};

typedef struct postlist_iterator {
	/* decompress function */
	postlist_rebuf_callbk    on_rebuf;
	/* iterator read buffer */
	invlist_node_codec_buf_t rd_buf;
} *postlist_iter_t;

///* main functions */
//struct postlist *
//postlist_create(uint32_t, uint32_t, uint32_t, void *,
//                struct postlist_callbks);
//
//void postlist_free(struct postlist*);
//
//void postlist_print_info(struct postlist*);
//
//size_t postlist_write(struct postlist*, const void*, size_t);
//size_t postlist_write_complete(struct postlist*);
//
//void print_postlist(struct postlist *);
//
///* postlist_iterator functions */
//postlist_iter_t postlist_iterator(struct postlist*);
//int             postlist_empty(struct postlist*);
//int             postlist_iter_next(struct postlist_iterator*);
//uint64_t        postlist_iter_cur(struct postlist_iterator*);
//void*           postlist_iter_cur_item(struct postlist_iterator*);
//int             postlist_iter_jump(struct postlist_iterator*, uint64_t);
//int             postlist_iter_jump32(struct postlist_iterator*, uint32_t);
//void            postlist_iter_free(struct postlist_iterator*);
