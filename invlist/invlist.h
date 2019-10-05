#pragma once
/* in memory inverted list implementation */

#include <stdint.h>
#include "skippy/skippy.h"
#include "skippy/ondisk-skippy.h"
#include "codec-buf.h"

#define MAX_PATH_LEN 2048

/* structures */
struct invlist_node {
	struct skippy_node  sn;
	char               *blk;
	uint16_t            len;
	uint16_t            size;
};

enum invlist_type {
	INVLIST_TYPE_ONDISK,
	INVLIST_TYPE_INMEMO
};

struct invlist {
	enum invlist_type        type;
	union {
		struct invlist_node *head; /* in-memo invlist */
		char   path[MAX_PATH_LEN]; /* on-disk invlist */
	};

	size_t                   tot_sz; /* total size in memory in bytes */
	uint32_t                 n_blk; /* total number of blocks */

	struct skippy            skippy;
	uint32_t                 buf_max_len;
	uint32_t                 buf_max_sz;
	codec_buf_struct_info_t *c_info;
};

struct invlist_iterator;
typedef uint64_t (*buf_key_callbk)(struct invlist_iterator*, uint32_t);

typedef struct invlist_iterator {
	void                   **buf;
	uint32_t                 buf_idx;
	uint32_t                 buf_len;
	struct invlist          *invlist;

	codec_buf_struct_info_t *c_info;
	/* we need c_info here so that we can free
	 * iterator buf even after invlist gets destroyed. */

	buf_key_callbk           bufkey; /* key value map */

	struct invlist_node     *cur; /* in-memo invlist */
	FILE                    *lfh; /* on-disk invlist */
	struct skippy_fh         sfh; /* on-disk skiplist */
} *invlist_iter_t;

/* invlist functions */
struct invlist
*invlist_open(const char*, uint32_t, codec_buf_struct_info_t*);
int  invlist_empty(struct invlist*);
void invlist_free(struct invlist*);

/* writer functions */
invlist_iter_t invlist_writer(struct invlist*);
// Note: Use invlist_iter_free() to free writer.

size_t invlist_writer_flush(struct invlist_iterator*);
size_t invlist_writer_write(struct invlist_iterator*, const void*);

/* reader functions */
invlist_iter_t invlist_iterator(struct invlist*);
void           invlist_iter_free(struct invlist_iterator*);

int      invlist_iter_next(struct invlist_iterator*);
size_t   invlist_iter_read(struct invlist_iterator*, void*);

int invlist_iter_jump(struct invlist_iterator*, uint64_t);

/* misc function */
void invlist_print_as_decoded_ints(struct invlist*);
void iterator_set_bufkey_to_32(invlist_iter_t);
