#pragma once
#include "codec/codec.h"
#include "skippy.h"

#define MEM_POSTING_PAGE_SZ          4096
#define MEM_POSTING_PAGES_PER_BLOCK  2

#define MEM_POSTING_BLOCK_SZ (MEM_POSTING_PAGE_SZ * MEM_POSTING_PAGES_PER_BLOCK)

struct mem_posting_blk {
	struct skippy_node       sn;
	uint32_t                 end;
	uint32_t                 n_encode;
	char                     buff[MEM_POSTING_BLOCK_SZ];
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

bool mem_posting_paging(struct mem_posting*, size_t);

size_t
mem_posting_write(struct mem_posting*, uint32_t, const void*, size_t);

size_t
mem_posting_encode_tail(struct mem_posting*, struct codec*, size_t);

#define mem_posting_encode_flush mem_posting_encode_tail

void mem_posting_print(struct mem_posting*);

void
mem_posting_encode_struct(struct mem_posting*, uint32_t,
                          const void*, size_t, struct codec*);
