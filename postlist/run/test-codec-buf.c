#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mhook/mhook.h"

#include "codec/codec.h"
#include "codec-buf.h"

struct math_invlist_item {
	uint32_t docID;
	uint32_t secID;
	uint8_t  sect_width;
	uint8_t  orig_width;
	uint32_t symbinfo_offset;
};

/* field index for math_invlist_item */
enum {
	FI_DOCID,
	FI_SECID,
	FI_SECT_WIDTH,
	FI_ORIG_WIDTH,
	FI_OFFSET
};

static void print_item(struct math_invlist_item *item)
{
	printf("#%u, #%u, w%u, w%u, o%u\n", item->docID, item->secID,
		item->sect_width, item->orig_width, item->symbinfo_offset);
}

int main()
{
	/* register structure info for codec algorithm */
	struct codec_buf_struct_info *info = codec_buf_struct_info_alloc(5);

#define SET_FIELD_INFO(_idx, _name, _codec) \
	info->field_info[_idx] = FIELD_INFO(struct math_invlist_item, _name, _codec)

	SET_FIELD_INFO(FI_DOCID, docID, CODEC_FOR_DELTA);
	SET_FIELD_INFO(FI_SECID, secID, CODEC_FOR);
	SET_FIELD_INFO(FI_SECT_WIDTH, sect_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_ORIG_WIDTH, orig_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_OFFSET, symbinfo_offset, CODEC_FOR_DELTA);

#define N 64
	/* create a buffer to be compressed */
	void **buf = codec_buf_alloc(N, info);

	srand(time(0));
	struct math_invlist_item item[N] = {0};
	uint last_docID = 1;
	uint last_offset = 0;
	for (int i = 0; i < N; i++) {
		item[i].docID = last_docID + rand() % 10;
		item[i].secID = rand() % 10;
		item[i].sect_width = rand() % 6;
		item[i].orig_width = rand() % 64;
		item[i].symbinfo_offset = last_offset + rand() % 128;

		printf("writing ");
		print_item(item + i);
		codec_buf_set(buf, i, item + i, info);

		last_docID = item[i].docID;
		last_offset = item[i].symbinfo_offset;
	}

	/* compressing */
	size_t in_sz = sizeof(item[0]) * N;
	void *block = malloc(in_sz);
	size_t out_sz = codec_buf_encode(block, buf, N, info);
	printf("in sz = %lu, out block sz = %lu\n", in_sz, out_sz);

	/* decompressing */
	void **buf_check = codec_buf_alloc(N, info);
	out_sz = codec_buf_decode(buf_check, block, N, info);
	printf("decoded sz = %lu\n", out_sz);
	free(block);

	/* checking decompressed data */
	struct math_invlist_item item_check;
	for (int i = 0; i < N; i++) {
		CODEC_BUF_GET(item_check.docID, buf_check, FI_DOCID, i, info);
		CODEC_BUF_GET(item_check.secID, buf_check, FI_SECID, i, info);
		CODEC_BUF_GET(item_check.sect_width, buf_check, FI_SECT_WIDTH, i, info);
		CODEC_BUF_GET(item_check.orig_width, buf_check, FI_ORIG_WIDTH, i, info);
		CODEC_BUF_GET(item_check.symbinfo_offset, buf_check, FI_OFFSET, i, info);

		printf("read ");
		print_item(&item_check);

		if (memcmp(item + i, &item_check, sizeof item_check) == 0)
			prinfo("pass.");
		else
			prerr("failed.");
	}
	codec_buf_free(buf_check, info);

	/* free resources */
	codec_buf_free(buf, info);
	codec_buf_struct_info_free(info);

	mhook_print_unfree();
	return 0;
}
