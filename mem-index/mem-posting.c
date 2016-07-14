#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mem-posting.h"
#include "config.h"

/* include assert */
#ifdef DEBUG_MEM_POSTING
#undef N_DEBUG
#else
#define N_DEBUG
#endif
#include <assert.h>

#ifdef  DEBUG_MEM_POSTING_SMALL_BUF_SZ
#undef  MEM_POSTING_BUF_SZ
#define MEM_POSTING_BUF_SZ DEBUG_MEM_POSTING_SMALL_BUF_SZ
#endif

/* buffer setup/free macro */
#define SETUP_BUFFER(_po) \
	if (_po->buf == NULL) { \
		_po->buf = malloc(MEM_POSTING_BUF_SZ); \
		_po->tot_sz += MEM_POSTING_BUF_SZ; \
	} do {} while (0)

#define FREE_BUFFER(_po) \
	if (_po->buf) { \
		free(_po->buf); \
		_po->buf = NULL; \
		_po->tot_sz -= MEM_POSTING_BUF_SZ; \
	} do {} while (0)

struct mem_posting_callbks mem_term_posting_plain_calls()
{
	struct mem_posting_callbks ret = {
		onflush_for_plainpost,
		onrebuf_for_plainpost,
		getposarr_for_termpost
	};

	return ret;
}

struct mem_posting_callbks mem_term_posting_codec_calls()
{
	struct mem_posting_callbks ret = {
		onflush_for_termpost,
		onrebuf_for_termpost,
		getposarr_for_termpost,
	};

	return ret;
};

struct mem_posting_callbks mem_term_posting_with_pos_codec_calls()
{
	struct mem_posting_callbks ret = {
		onflush_for_termpost_with_pos,
		onrebuf_for_termpost_with_pos,
		getposarr_for_termpost_with_pos
	};

	return ret;
};

void mem_posting_print_info(struct mem_posting *po)
{
	printf("==== memory posting list info ====\n");
	printf("%u blocks (%.2f KB).\n", po->n_blk,
	       (float)po->tot_sz / 1024.f);

	skippy_print(&po->skippy);
}

struct mem_posting *mem_posting_create(uint32_t skippy_spans,
                                       struct mem_posting_callbks calls)
{
	struct mem_posting *ret;
	ret = malloc(sizeof(struct mem_posting));

	/* assign initial values */
	ret->head = ret->tail = NULL;
	ret->tot_sz = sizeof(struct mem_posting);
	ret->n_blk  = 0;
	skippy_init(&ret->skippy, skippy_spans);

	ret->buf = NULL;
	ret->buf_end = 0;

	ret->on_flush = calls.on_flush;
	ret->on_rebuf = calls.on_rebuf;
	ret->get_pos_arr = calls.get_pos_arr;

	/* leave iterator-related initializations to mem_posting_start() */

	return ret;
}

static struct mem_posting_node *create_node(uint32_t key, size_t size)
{
	struct mem_posting_node *ret;
	ret = malloc(sizeof(struct mem_posting_node));

	/* assign initial values */
	skippy_node_init(&ret->sn, key);
	ret->blk = malloc(size);
	ret->blk_sz = size;

	return ret;
}

static void
append_node(struct mem_posting *po, struct mem_posting_node *node)
{
	if (po->head == NULL)
		po->head = node;

	po->tail = node;
	po->tot_sz += node->blk_sz;
	po->n_blk ++;

#ifdef DEBUG_MEM_POSTING
	printf("appending skippy node...\n");
#endif
	skippy_append(&po->skippy, &node->sn);
}

static uint32_t mem_posting_flush(struct mem_posting *po)
{
	uint32_t flush_key, flush_sz;
	struct mem_posting_node *node;

	/* invoke flush callback and get flush size */
	flush_key = po->on_flush(po->buf, &po->buf_end);
	flush_sz = po->buf_end;

	/* append new node with copy of current buffer */
	node = create_node(flush_key, flush_sz);
	append_node(po, node);

	/* flush to new tail node */
	memcpy(node->blk, po->buf, flush_sz);

	/* reset buffer */
	po->buf_end = 0;

	return flush_sz;
}

size_t
mem_posting_write(struct mem_posting *po, const void *in, size_t size)
{
	size_t flush_sz = 0;

	/* setup buffer for writting */
	SETUP_BUFFER(po);

	/* flush buffer under inefficient buffer space */
	if (po->buf_end + size > MEM_POSTING_BUF_SZ)
		flush_sz = mem_posting_flush(po);

	/* write into buffer */
	assert(po->buf_end + size <= MEM_POSTING_BUF_SZ);
	memcpy(po->buf + po->buf_end, in, size);
	po->buf_end += size;

#ifdef DEBUG_MEM_POSTING
	printf("buffer after writting: [%u/%u]\n",
	       po->buf_end, MEM_POSTING_BUF_SZ);
#endif

	return flush_sz;
}

size_t mem_posting_write_complete(struct mem_posting *po)
{
	size_t flush_sz;

	if (po->buf)
		flush_sz = mem_posting_flush(po);

	FREE_BUFFER(po);
	return flush_sz;
}

void mem_posting_free(struct mem_posting *po)
{
	skippy_free(&po->skippy, struct mem_posting_node, sn,
	            free(p->blk); free(p));
	free(po->buf);
	free(po);
}

static void forward_cur(struct mem_posting_node **cur)
{
	/* update `cur' variables */
	*cur = MEMBER_2_STRUCT((*cur)->sn.next[0], struct mem_posting_node, sn);
}

static void rebuf_cur(struct mem_posting *po)
{
	struct mem_posting_node *cur = po->cur;

	if (cur) {
		/* refill buffer */
		memcpy(po->buf, cur->blk, cur->blk_sz);
		po->buf_end = cur->blk_sz;

		/* invoke rebuf callback */
		po->on_rebuf(po->buf, &po->buf_end);

	} else {
		/* reset buffer variables anyway */
		po->buf_end = 0;
	}

	po->buf_idx = 0;
}

bool mem_posting_start(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;

	if (po->head == NULL)
		return 0;

	/* setup buffer for rebuffering */
	SETUP_BUFFER(po);

	/* initial buffer filling */
	po->cur = po->head;
	rebuf_cur(po);

	return (po->buf_end != 0);
}

bool mem_posting_next(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	size_t  pos_arr_sz;
	char   *pos_arr;
	pos_arr = po->get_pos_arr(po->buf + po->buf_idx, &pos_arr_sz);
	po->buf_idx = (uint32_t)(pos_arr - po->buf) + pos_arr_sz;

	do {
		if (po->buf_idx < po->buf_end) {
			return 1;
		} else {
			forward_cur(&po->cur);
			rebuf_cur(po);
		}

	} while (po->buf_end != 0);

	return 0;
}

void* mem_posting_cur_item(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	return po->buf + po->buf_idx;
}

uint64_t mem_posting_cur_item_id(void *item)
{
	uint32_t *curID = (uint32_t*)item;
	return (uint64_t)(*curID);
}

bool mem_posting_jump(void *po_, uint64_t target_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	uint32_t            target = (uint32_t)target_;
	struct skippy_node *jump_to;
	uint32_t           *curID;

	/* using skip-list */
	jump_to = skippy_node_jump(&po->cur->sn, target);

	if (jump_to != &po->cur->sn) {
		/* if we can jump over some blocks */
#ifdef DEBUG_MEM_POSTING
		printf("jump to a different node.\n");
#endif
		po->cur = MEMBER_2_STRUCT(jump_to, struct mem_posting_node, sn);
		rebuf_cur(po);
	}
#ifdef DEBUG_MEM_POSTING
	else
		printf("stay in the same node.\n");
#endif

	/* at this point, there shouldn't be any problem to access
	 * current posting item:
	 * case 1: if had jumped to a new block, the buffer must
	 * be non-empty.
	 * case 2: if we stay in the old block, it is guaranteed
	 * that current posting item is a valid pointer. */

	/* seek in the current block until we get to an ID greater or
	 * equal to target ID. */
	do {
		/* docID must be the first member of structure */
		curID = (uint32_t*)mem_posting_cur_item(po);

		if (*curID >= target) {
			return 1;
		}

	} while (mem_posting_next(po));

	return 0;
}

void mem_posting_finish(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	FREE_BUFFER(po);
}

position_t *mem_posting_cur_pos_arr(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;

	size_t      pos_arr_sz;
	char       *pos_arr;
	position_t *copy;

	pos_arr = po->get_pos_arr(po->buf + po->buf_idx, &pos_arr_sz);

	copy = malloc(pos_arr_sz);
	memcpy(copy, pos_arr, pos_arr_sz);

	return copy;
}
