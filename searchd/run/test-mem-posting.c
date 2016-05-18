#include <stdio.h>
#include <stdlib.h>
#include "skippy.h"

#define MEM_POSTING_PAGE_SZ          4096
#define MEM_POSTING_PAGES_PER_BLOCK  2

#define MEM_POSTING_BLOCK_SZ (MEM_POSTING_PAGE_SZ * MEM_POSTING_PAGES_PER_BLOCK)

struct mem_posting_blk {
	struct skippy_node       sn;
	uint32_t                 end;
	char                     buff[MEM_POSTING_BLOCK_SZ];
	struct mem_posting_blk  *next;
};

struct mem_posting {
	uint32_t                   n_used_bytes;
	uint32_t                   n_tot_blocks;
	struct mem_posting_blk    *head, *tail;
	struct skippy              skippy;
};

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
		free(p); printf("free p\n");
		p = save;
	}

	free(po); printf("free po\n");
}

struct mem_posting_blk *mem_posting_creatblk(uint32_t key)
{
	struct mem_posting_blk *ret;
	ret = malloc(sizeof(struct mem_posting_blk));

	skippy_node_init(&ret->sn, key);

	ret->end = 0;
	ret->next = NULL;

	return ret;
}

int
mem_posting_append_blk(struct mem_posting *po, struct mem_posting_blk *blk)
{
	if (po->head == NULL)
		po->head = blk;
	else
		po->tail->next = blk;

	po->tail = blk;

	po->n_tot_blocks ++;

	return 0;
}

size_t
mem_posting_write(struct mem_posting *po, uint32_t first_key,
                  const void *in, size_t bytes)
{
	struct mem_posting_blk *blk;

	if (po->tail == NULL /* no block created yet */ ||
	    po->tail->end + bytes > MEM_POSTING_BLOCK_SZ /* not enough space */) {

		blk = mem_posting_creatblk(first_key);
		if (blk == NULL)
			return 0;

		skippy_append(&po->skippy, &blk->sn);
		mem_posting_append_blk(po, blk);

		printf("append\n");

		return mem_posting_write(po, first_key, in, bytes);
	}

	memcpy(po->tail->buff + po->tail->end, in, bytes);
	po->tail->end += bytes;
	po->n_used_bytes += bytes;

	return 0;
}

static void print_ascii_buff(char *buff, size_t bytes)
{
	int i;
	for (i = 0; i < bytes; i++)
		printf("%c", buff[i]);
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
		print_ascii_buff(blk->buff, blk->end);

		blk = blk->next;
		i ++;
	}
}

int main()
{
	int i, j, n;
	size_t sz, total_sz = 0;
	struct mem_posting *po = NULL;
	char test_str[MEM_POSTING_BLOCK_SZ];

	po = mem_posting_create(2);
	printf("one block %u bytes.\n", MEM_POSTING_BLOCK_SZ);

	for (n = 0; n < 5; n++)
	for (i = 0; i < 26; i++) {
		sz = 5 + i * 10;
		for (j = 0; j < sz; j++) {
			if (j + 1 == sz)
				test_str[j] = 'Z';
			else
				test_str[j] = 'a' + i;
		}

		mem_posting_write(po, i, test_str, sz);

		total_sz += sz;
	}

	printf("total %lu bytes...\n", total_sz);

	mem_posting_print(po);
	mem_posting_release(po);
	return 0;
}
