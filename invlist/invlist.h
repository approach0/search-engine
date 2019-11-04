#pragma once
/* in memory inverted list implementation */

#include <stdint.h>
#include "skippy/skippy.h"
#include "skippy/ondisk-skippy.h"
#include "codec-buf.h"
#include "config.h"

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

struct invlist_iterator;
typedef uint64_t buf_key_callbk(struct invlist_iterator*, uint32_t);

/* default bufkey callback */
buf_key_callbk invlist_iter_default_bufkey;

/* inverted list */
struct invlist {
	enum invlist_type        type;
	union {
		struct invlist_node *head; /* in-memo invlist */
		char   path[MAX_PATH_LEN]; /* on-disk invlist */
	};

	size_t                   tot_payload_sz; /* total payload size in bytes */
	uint32_t                 n_blk; /* total number of blocks */

	struct skippy            skippy;
	uint32_t                 buf_max_len;
	uint32_t                 buf_max_sz;

	codec_buf_struct_info_t *c_info;
	buf_key_callbk          *bufkey;
};

/* inverted list iterator */
typedef struct invlist_iterator {
	void                   **buf;
	uint32_t                 buf_idx;
	uint32_t                 buf_len;
	struct invlist          *invlist;
	enum invlist_type        type;
	char                    *path;
	uint32_t                 buf_max_len;
	uint32_t                 buf_max_sz;

	codec_buf_struct_info_t *c_info;
	/* we need c_info here so that we can free
	 * iterator buf even after invlist gets destroyed. */

	buf_key_callbk          *bufkey; /* key value callback */

	struct invlist_node     *cur; /* in-memo invlist */

	/* below only used for on-disk reading */
	struct skippy_fh         sfh;
	FILE                    *lfh;
	int                      if_disk_buf_read;
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
uint64_t invlist_iter_bufkey(struct invlist_iterator*, uint32_t);
uint64_t invlist_iter_curkey(struct invlist_iterator*);
size_t   invlist_iter_read(struct invlist_iterator*, void*);

int invlist_iter_jump(struct invlist_iterator*, uint64_t);

/* misc function */
void invlist_iter_print_as_decoded_ints(invlist_iter_t);
void invlist_print_as_decoded_ints(struct invlist*);
