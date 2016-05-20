#include <stdlib.h>
#include <stdio.h>

#include "mem-posting.h"
#include "config.h"

#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef void (*blk_print_callbk)(struct mem_posting_blk*, size_t);

static void po_init(struct mem_posting *po, uint32_t skippy_spans)
{
	po->n_used_bytes = 0;
	po->n_tot_blocks = 0;
	po->head = NULL; /* lazy, create on write */
	po->tail = NULL;
	skippy_init(&po->skippy, skippy_spans);
}

struct mem_posting *mem_posting_create(uint32_t skippy_spans)
{
	struct mem_posting *ret;
	ret = malloc(sizeof(struct mem_posting));
	po_init(ret, skippy_spans);

#ifdef DEBUG_MEM_POSTING
	printf("new memory posting list created.\n");
#endif
	return ret;
}

void mem_posting_clear(struct mem_posting *po)
{
	uint32_t skippy_spans = po->skippy.n_spans;
	struct mem_posting_blk *save, *p = po->head;

	while (p) {
		save = p->next;
#ifdef DEBUG_MEM_POSTING
	printf("free memory posting list block.\n");
#endif
		free(p);
		p = save;
	}
	//Or use skippy_free(&po->skippy, struct mem_posting_blk, sn);

	po_init(po, skippy_spans);
}

void mem_posting_release(struct mem_posting *po)
{
	mem_posting_clear(po);

#ifdef DEBUG_MEM_POSTING
	printf("free memory posting list.\n");
#endif
	free(po);
}

static struct mem_posting_blk *create_blk(uint32_t key)
{
	struct mem_posting_blk *ret;
	ret = malloc(sizeof(struct mem_posting_blk));

	skippy_node_init(&ret->sn, key);

	ret->end = 0;
	ret->n_writes = 0;
	ret->next = NULL;

	return ret;
}

static int
append_blk(struct mem_posting *po, struct mem_posting_blk *blk)
{
	if (po->head == NULL)
		po->head = blk;
	else
		po->tail->next = blk;

	po->tail = blk;

	po->n_tot_blocks ++;

#ifdef DEBUG_MEM_POSTING
	printf("new block appended to memory posting list.\n");
#endif
	return 0;
}

static __inline bool is_paging(struct mem_posting *po, size_t next_bytes)
{
	return (po->tail == NULL /* no block created yet */ ||
	        po->tail->end + next_bytes > MEM_POSTING_BLOCK_SZ
	        /* not enough space */);
}

uint32_t
mem_posting_encode(struct mem_posting *dest, struct mem_posting *src,
                   size_t struct_sz, struct codec *codecs)
{
	struct mem_posting_blk *srcblk = src->head;
	const uint32_t n_encode = MAX(1, MEM_POSTING_N_ENCODE);
	const uint32_t n_encode_bytes = n_encode * struct_sz;

	uint32_t  byte_now, first_key, res_bytes, remain_bytes, n_enc_remain;
	char      buf[MEM_POSTING_BLOCK_SZ];
	enc_hd_t *buf_head = (enc_hd_t*)buf;
	enc_hd_t *buf_load = buf_head + 1;

	while (srcblk) {
		/* for each of source block in order */
		byte_now = 0;
		n_enc_remain = srcblk->n_writes;

		while (byte_now < srcblk->end) {
			/* for each N structures in this source block */
			first_key = *(uint32_t*)(srcblk->buff + byte_now);
			remain_bytes = srcblk->end - byte_now;

			if (remain_bytes >= n_encode_bytes) {
				/* encode n_encode structures */
				res_bytes = encode_struct_arr(buf_load,
				                              srcblk->buff + byte_now,
				                              codecs, n_encode, struct_sz);
				byte_now += n_encode_bytes;
				n_enc_remain -= n_encode;

				/* write encode header */
				*buf_head = n_encode;
			} else {
				/* encode n_enc_remain structures */
				res_bytes = encode_struct_arr(buf_load,
				                              srcblk->buff + byte_now,
				                              codecs, n_enc_remain, struct_sz);
				byte_now += remain_bytes;

				/* write encode header */
				*buf_head = n_enc_remain;
			}

			/* write encoded bytes to destination memory posting */
			mem_posting_write(dest, first_key, buf,
			                  sizeof(enc_hd_t) + res_bytes);
		}

		srcblk = srcblk->next;
	}

	return n_encode;
}

size_t
mem_posting_write(struct mem_posting *po, uint32_t first_key,
                  const void *in, size_t bytes)
{
	struct mem_posting_blk *blk;

	if (is_paging(po, bytes)) {

		blk = create_blk(first_key);
		if (blk == NULL)
			return 0;

		skippy_append(&po->skippy, &blk->sn);
		append_blk(po, blk);
		return mem_posting_write(po, first_key, in, bytes);
	}

	memcpy(po->tail->buff + po->tail->end, in, bytes);
	po->tail->end += bytes;
	po->tail->n_writes ++;
	po->n_used_bytes += bytes;

	return 0;
}

static void print_int32_buff(uint32_t *integer, size_t n)
{
	int i;
	for (i = 0; i < n; i++)
		printf("%u,", integer[i]);
	printf("\n");
}

static void po_print(struct mem_posting *po, blk_print_callbk blk_print_fun,
                     size_t struct_sz)
{
	struct mem_posting_blk *blk = po->head;
	int i = 0;
	printf("==== memory posting list (%u blocks, %u/%u used) ====\n",
	       po->n_tot_blocks, po->n_used_bytes,
	       po->n_tot_blocks * MEM_POSTING_BLOCK_SZ);

	skippy_print(&po->skippy);

	while (blk) {
		printf("block #%d ", i);
		if (blk == po->head)
			printf("(head) ");
		else if (blk == po->tail)
			printf("(tail) ");

		printf("[%u/%d used]:\n", blk->end, MEM_POSTING_BLOCK_SZ);

		skippy_node_print(&blk->sn);
		blk_print_fun(blk, struct_sz);

		blk = blk->next;
		i ++;
	}
}

static void po_print_callbk(struct mem_posting_blk* blk, size_t struct_sz)
{
	print_int32_buff((uint32_t*)blk->buff, blk->end / sizeof(uint32_t));
}

void mem_posting_print(struct mem_posting *po)
{
	po_print(po, &po_print_callbk, 0);
}

static void po_enc_print_callbk(struct mem_posting_blk* blk, size_t struct_sz)
{
	int j = 0;
	enc_hd_t *enc_head;
	uint32_t n_members = struct_sz / sizeof(uint32_t);

	while (j < blk->end) {
		enc_head = (enc_hd_t*)(blk->buff + j);
		printf("[ %u ]:", *enc_head);
		j += sizeof(enc_hd_t);
		print_int32_buff((uint32_t*)(blk->buff + j),
				(*enc_head) * n_members);
		j += (*enc_head) * n_members * sizeof(uint32_t);
	}
}

void mem_posting_enc_print(struct mem_posting *po, size_t struct_sz)
{
	po_print(po, &po_enc_print_callbk, struct_sz);
}

bool mem_posting_start(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	return 0;
}

bool mem_posting_jump(void *po_, uint64_t target)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	return 0;
}

bool mem_posting_next(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	return 0;
}

void mem_posting_finish(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
}

void* mem_posting_current(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	return NULL;
}
