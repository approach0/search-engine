#include <stdlib.h>
#include <stdio.h>

#include "mem-posting.h"

struct mem_posting *mem_posting_create(uint32_t skippy_spans)
{
	struct mem_posting *ret;
	ret = malloc(sizeof(struct mem_posting));

	ret->n_used_bytes = 0;
	ret->n_tot_blocks = 0;
	ret->head = NULL; /* lazy, create on write */
	ret->tail = NULL;
	skippy_init(&ret->skippy, skippy_spans);

	return ret;
}

void mem_posting_release(struct mem_posting *po)
{
	struct mem_posting_blk *save, *p = po->head;

	while (p) {
		save = p->next;
		free(p);
		p = save;
	}

	free(po);
}

static struct mem_posting_blk *create_blk(uint32_t key)
{
	struct mem_posting_blk *ret;
	ret = malloc(sizeof(struct mem_posting_blk));

	skippy_node_init(&ret->sn, key);

	ret->end = 0;
	ret->n_encode = 0;
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

	return 0;
}

size_t mem_posting_encode_tail(struct mem_posting *po, struct codec* codecs,
                               size_t struct_sz)
{
	struct mem_posting_blk *tail = po->tail;
	char   tmp_buf[MEM_POSTING_BLOCK_SZ];

	if (tail == NULL)
		return 0;

	tail->n_encode = tail->end / struct_sz;
	tail->end = encode_struct_arr(tmp_buf, tail->buff, codecs, tail->n_encode,
	                              struct_sz);
	memcpy(tail->buff, tmp_buf, tail->end);

	return tail->end;
}

bool mem_posting_paging(struct mem_posting *po, size_t next_bytes)
{
	return (po->tail == NULL /* no block created yet */ ||
	        po->tail->end + next_bytes > MEM_POSTING_BLOCK_SZ
	        /* not enough space */);
}

size_t
mem_posting_write(struct mem_posting *po, uint32_t first_key,
                  const void *in, size_t bytes)
{
	struct mem_posting_blk *blk;

	if (mem_posting_paging(po, bytes)) {

		blk = create_blk(first_key);
		if (blk == NULL)
			return 0;

		skippy_append(&po->skippy, &blk->sn);
		append_blk(po, blk);
		return mem_posting_write(po, first_key, in, bytes);
	}

	memcpy(po->tail->buff + po->tail->end, in, bytes);
	po->tail->end += bytes;
	po->n_used_bytes += bytes;

	return 0;
}

void
mem_posting_encode_struct(struct mem_posting *po, uint32_t first_key,
                          const void *item, size_t struct_sz,
                          struct codec *codecs)
{
	if (mem_posting_paging(po, struct_sz))
		mem_posting_encode_tail(po, codecs, struct_sz);

	mem_posting_write(po, first_key, item, struct_sz);
}

static void print_int32_buff(uint32_t *integer, size_t n)
{
	int i;
	for (i = 0; i < n; i++)
		printf("%u,", integer[i]);
	printf("\n");
}

void
mem_posting_print(struct mem_posting *po)
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
		print_int32_buff((uint32_t*)blk->buff, blk->end / 4);

		blk = blk->next;
		i ++;
	}
}
