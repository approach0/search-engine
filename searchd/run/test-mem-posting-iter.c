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
	uint32_t docID;
	struct math_posting_item item, *cur_item;
	struct mem_posting *po, *buf_po;
	struct codec codecs[] = {
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL}
	};

	/* create posting lists */
	po = mem_posting_create(2);
	buf_po = mem_posting_create(MAX_SKIPPY_SPANS);

	/* write some dummy values */
	mem_posting_set_enc(po, sizeof(struct math_posting_item), codecs);

	for (docID = 1; docID < N; docID++) {
		item.doc_id = docID;
		item.exp_id = docID % 5;
		item.pathinfo_pos = docID * 100;

		mem_posting_write(buf_po, 0, &item, sizeof(struct math_posting_item));
	}

	/* encode */
	mem_posting_encode(po, buf_po);
	mem_posting_release(buf_po);

	/* now we get the encoded posting list */
	printf("encoded posting list:\n");
	mem_posting_enc_print(po);

	if (!mem_posting_start(po)) {
		printf("error in starting posting merge.\n");
		goto finish;
	}

	do {
		cur_item = (struct math_posting_item*)mem_posting_current(po);
		printf("current item [%u, %u, %u]\n", cur_item->doc_id,
		       cur_item->exp_id, cur_item->pathinfo_pos);
	} while (mem_posting_next(po));

finish:
	mem_posting_finish(po);

	/* release posting lists */
	mem_posting_release(po);
	return 0;
}
