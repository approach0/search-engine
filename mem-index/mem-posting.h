#pragma once
#include <stdint.h>
#include "term-index/term-index.h" /* for position_t */
#include "mem-posting-calls.h"
#include "skippy.h"

/* callback types */
typedef uint32_t (*mem_posting_flush_callbk)(char*, uint32_t*);
typedef void     (*mem_posting_rebuf_callbk)(char*, uint32_t*);
typedef char    *(*mem_posting_pos_arr_callbk)(char*, size_t*);

struct mem_posting_callbks {
	mem_posting_flush_callbk   on_flush;
	mem_posting_rebuf_callbk   on_rebuf;
	mem_posting_pos_arr_callbk get_pos_arr;
};

/* some mem_posting_callbks setup utility functions */
struct mem_posting_callbks mem_term_posting_plain_calls();
struct mem_posting_callbks mem_term_posting_codec_calls();
struct mem_posting_callbks mem_term_posting_with_pos_codec_calls();

/* structures */
struct mem_posting_node {
	struct skippy_node  sn;
	char               *blk;
	size_t              blk_sz;
};

struct mem_posting {
	struct mem_posting_node *head, *tail;
	size_t                   tot_sz;
	uint32_t                 n_blk;
	struct skippy            skippy;

	/* writer/iterator buffer */
	char                    *buf;
	uint32_t                 buf_end;

	/* callback functions */
	mem_posting_flush_callbk   on_flush;
	mem_posting_rebuf_callbk   on_rebuf;
	mem_posting_pos_arr_callbk get_pos_arr;

	/* iterator-related */
	struct mem_posting_node *cur;
	uint32_t                 buf_idx;
};

/* main functions */
struct mem_posting *mem_posting_create(uint32_t, struct mem_posting_callbks);
void mem_posting_free(struct mem_posting*);

void mem_posting_print_info(struct mem_posting*);

size_t mem_posting_write(struct mem_posting*, const void*, size_t);
size_t mem_posting_write_complete(struct mem_posting*);

/* iterator functions */
bool  mem_posting_start(void*);
bool  mem_posting_next(void*);
void* mem_posting_current(void*);
uint64_t mem_posting_current_id(void*);
bool  mem_posting_jump(void*, uint64_t);
void  mem_posting_finish(void*);

position_t *mem_posting_cur_pos_arr(void*);
