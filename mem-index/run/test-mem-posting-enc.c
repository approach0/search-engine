#include <stdio.h>
#include <stdlib.h>

#include "math-index/math-index.h"
#include "mem-posting.h"

int main(void)
{
#ifdef DEBUG_MEM_POSTING
	const uint32_t N = 10;
#else
	const uint32_t N = 2500;
#endif
	uint32_t i, docID;
	struct math_posting_item item;
	struct mem_posting *po, *buf_po;

#if 0
	struct codec *codecs[] = {
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS)
	};
#else
	struct codec *codecs[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS)
	};
#endif

	printf("%u bytes/block.\n", MEM_POSTING_BLOCK_SZ);

	po = mem_posting_create(sizeof(struct math_posting_item), 2);
	buf_po = mem_posting_create(sizeof(struct math_posting_item),
	                            MAX_SKIPPY_SPANS);
	mem_posting_set_codecs(po, codecs);

	for (docID = 1; docID < N; docID++) {
		item.doc_id = docID;
		item.exp_id = docID % 5;
		item.pathinfo_pos = docID * 100;

		printf("writting (%u, %u, %u)...\n",
		       item.doc_id, item.exp_id, item.pathinfo_pos);
		mem_posting_write(buf_po, 0, &item, sizeof(struct math_posting_item));

		if (docID == N / 2 || docID + 1 == N) {
			printf("\nencoding...\n");
			mem_posting_encode(po, buf_po);
			printf("\n");

			mem_posting_clear(buf_po);
		}
	}

	printf("raw posting list:\n");
	mem_posting_print_raw(po);

	printf("decoded posting list:\n");
	mem_posting_print_dec(po);

	for (i = 0; i < sizeof(codecs) / sizeof(struct codec*); i++)
		codec_free((void*)codecs[i]);

	mem_posting_release(po);
	mem_posting_release(buf_po);
	return 0;
}
