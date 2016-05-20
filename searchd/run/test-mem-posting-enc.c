#include <stdio.h>
#include <stdlib.h>

#include "math-index/math-index.h"
#include "mem-posting.h"

int main()
{
#ifdef DEBUG_MEM_POSTING
	const int N = 60;
#else
	const int N = 2500;
#endif
	uint32_t docID, n_encode;
	struct math_posting_item item;
	struct mem_posting *po, *buf_po;
	struct codec codecs[] = {
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL}
	};

	printf("%u bytes/block.\n", MEM_POSTING_BLOCK_SZ);

	po = mem_posting_create(2);
	buf_po = mem_posting_create(MAX_SKIPPY_SPANS);

	for (docID = 1; docID < N; docID++) {
		item.doc_id = docID;
		item.exp_id = docID % 5;
		item.pathinfo_pos = docID * 100;

		mem_posting_write(buf_po, 0, &item, sizeof(struct math_posting_item));
		if (docID == N / 2 || docID + 1 == N) {
#ifdef DEBUG_MEM_POSTING
			printf("buffer posting list before clear:\n");
			mem_posting_print(buf_po);
#endif
			n_encode = mem_posting_encode(po, buf_po,
			           sizeof(struct math_posting_item), codecs);
			mem_posting_clear(buf_po);
			printf("use n_encode = %u\n", n_encode);

#ifdef DEBUG_MEM_POSTING
			printf("encoded posting list:\n");
			mem_posting_enc_print(po, sizeof(struct math_posting_item));
#endif
		}
	}

#ifndef DEBUG_MEM_POSTING
	printf("encoded posting list:\n");
	mem_posting_enc_print(po, sizeof(struct math_posting_item));
#endif

	mem_posting_release(po);
	mem_posting_release(buf_po);
	return 0;
}
