#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mhook/mhook.h"

#include "codec/codec.h"
#include "codec-buf.h"

struct math_invlist_item {
	uint32_t docID;
	uint32_t secID;
	uint16_t sect_width;
	uint16_t orig_width;
	uint32_t symbinfo_offset;
};

#define FI_DOCID           0
#define FI_SECID           1
#define FI_SECT_WIDTH      2
#define FI_ORIG_WIDTH      3
#define FI_SYMBINFO_OFFSET 4

int main()
{
	/* register structure info for codec algorithm */
	struct codec_buf_struct_info *info = codec_buf_struct_info_alloc(5);

#define SET_FIELD_INFO(_idx, _name, _codec) \
	info->field_info[_idx] = FIELD_INFO(struct math_invlist_item, _name, _codec)

	SET_FIELD_INFO(FI_DOCID, docID, CODEC_FOR_DELTA);
	SET_FIELD_INFO(FI_SECID, secID, CODEC_FOR);
	SET_FIELD_INFO(FI_SECT_WIDTH, sect_width, CODEC_FOR);
	SET_FIELD_INFO(FI_ORIG_WIDTH, orig_width, CODEC_FOR);
	SET_FIELD_INFO(FI_SYMBINFO_OFFSET, symbinfo_offset, CODEC_FOR_DELTA);

#define N 64
	/* create a buffer to be compressed */
	void **buf = codec_buf_alloc(N, info);

	srand(time(0));
	struct math_invlist_item item = {0};
	for (int i = 0; i < N; i++) {
		item.docID += rand() % 10;
		item.secID = rand() % 10;
		item.sect_width = rand() % 64;
		item.orig_width = rand() % 64;
		item.symbinfo_offset += rand() % 128;

		codec_buf_set(buf, i, &item, info);
	}

	/* compressing */
	size_t in_sz = sizeof item * N;
	void *block = malloc(in_sz);
	size_t out_sz = codec_buf_encode(block, buf, N, info);
	printf("in sz = %lu, out block sz = %lu\n", in_sz, out_sz);

	/* decompressing */
	void **buf_same = codec_buf_alloc(N, info);
	out_sz = codec_buf_decode(buf_same, block, N, info);
	codec_buf_free(buf_same, info);
	printf("decoded sz = %lu\n", out_sz);
	free(block);

	/* free resources */
	codec_buf_free(buf, info);
	codec_buf_struct_info_free(info);

	mhook_print_unfree();
	return 0;
}
