#pragma once
#include <stdint.h>
#include "skippy/skippy.h"

/* callback types */
typedef uint64_t (*postlist_flush_callbk)(char*, uint32_t*, void *);
typedef void     (*postlist_rebuf_callbk)(char*, uint32_t*, void *);
typedef void     (*postlist_free_callbk)(void *);

struct postlist_callbks {
	postlist_flush_callbk   on_flush;
	postlist_rebuf_callbk   on_rebuf;
	postlist_free_callbk    on_free;
};

/* structures */
struct postlist_node {
	struct skippy_node  sn;
	char               *blk;
	size_t              blk_sz;
};

struct postlist {
	struct postlist_node    *head, *tail;
	size_t                   tot_sz;
	uint32_t                 n_blk;
	struct skippy            skippy;
	uint32_t                 buf_sz; /* in bytes */

	/* callback functions */
	postlist_flush_callbk   on_flush;
	postlist_rebuf_callbk   on_rebuf;
	postlist_free_callbk    on_free;

	/* write buffer/iterator */
	struct postlist_node    *cur;
	char                    *buf;
	uint32_t                 buf_idx;
	uint32_t                 buf_end; /* in bytes */
	void                    *buf_arg; /* codec array for fields */
	uint32_t                 item_sz;
};

typedef struct postlist_iterator {
	/* current block */
	struct postlist_node   *cur;

	/* decompress function */
	postlist_rebuf_callbk   on_rebuf;
	void                    *buf_arg; /* codec */

	/* iterator buffer */
	char                   *buf;
	uint32_t                buf_idx;
	uint32_t                buf_end; /* in bytes */
	uint32_t                item_sz;
} *postlist_iter_t;

/* main functions */
struct postlist *
postlist_create(uint32_t, uint32_t, uint32_t, void *,
                struct postlist_callbks);

void postlist_free(struct postlist*);

void postlist_print_info(struct postlist*);

size_t postlist_write(struct postlist*, const void*, size_t);
size_t postlist_write_complete(struct postlist*);

void print_postlist(struct postlist *);

/* postlist_iterator functions */
postlist_iter_t postlist_iterator(struct postlist*);
int             postlist_empty(struct postlist*);
int             postlist_iter_next(struct postlist_iterator*);
uint64_t        postlist_iter_cur(struct postlist_iterator*);
void*           postlist_iter_cur_item(struct postlist_iterator*);
int             postlist_iter_jump(struct postlist_iterator*, uint64_t);
int             postlist_iter_jump32(struct postlist_iterator*, uint32_t);
void            postlist_iter_free(struct postlist_iterator*);
