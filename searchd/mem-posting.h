#pragma once
#include "codec/codec.h"
#include "skippy.h"
#include "config.h"

#ifdef DEBUG_MEM_POSTING
#define SYS_MEM_PAGE_SZ 64
#else
#define SYS_MEM_PAGE_SZ 4096
#endif

#define MEM_POSTING_PAGES_PER_BLOCK  2

#define MEM_POSTING_BLOCK_SZ (SYS_MEM_PAGE_SZ * MEM_POSTING_PAGES_PER_BLOCK)

#define MEM_POSTING_N_ENCODE ((MEM_POSTING_BLOCK_SZ / struct_sz) >> 2)

typedef uint16_t enc_hd_t; /* encode header */

struct mem_posting_blk {
	struct skippy_node       sn;
	uint32_t                 end;
	char                     buff[MEM_POSTING_BLOCK_SZ];
	uint32_t                 n_writes;
	struct mem_posting_blk  *next;
};

struct mem_posting {
	uint32_t                   n_used_bytes;
	uint32_t                   n_tot_blocks;
	struct mem_posting_blk    *head, *tail;
	struct skippy              skippy;
};

struct mem_posting *mem_posting_create(uint32_t);

void mem_posting_release(struct mem_posting*);

void mem_posting_clear(struct mem_posting*);

size_t
mem_posting_write(struct mem_posting*, uint32_t, const void*, size_t);

uint32_t
mem_posting_encode(struct mem_posting*, struct mem_posting*,
                   size_t, struct codec*);

void mem_posting_print(struct mem_posting*);

void mem_posting_enc_print(struct mem_posting*, size_t);

/* merge-related functions */
bool  mem_posting_start(void*);
bool  mem_posting_jump(void*, uint64_t);
bool  mem_posting_next(void*);
void  mem_posting_finish(void*);
void* mem_posting_current(void*);
