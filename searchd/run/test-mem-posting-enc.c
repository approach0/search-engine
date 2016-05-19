#include <stdio.h>
#include <stdlib.h>
#include "math-index/math-index.h"
#include "mem-posting.h"

int main()
{
	uint32_t docID;
	struct math_posting_item item;
	struct mem_posting *po;
	struct codec codecs[] = {
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL}
	};

	printf("%u bytes/block.\n", MEM_POSTING_BLOCK_SZ);

	po = mem_posting_create(2);

	for (docID = 0; docID < 1000; docID++) {
		item.doc_id = docID;
		item.exp_id = docID % 5;
		item.pathinfo_pos = docID * 100;

		mem_posting_encode_struct(po, docID, &item,
		                          sizeof(struct math_posting_item), codecs);
	}

	mem_posting_encode_flush(po, codecs, sizeof(struct math_posting_item));

	mem_posting_print(po);
	mem_posting_release(po);
	return 0;
}
