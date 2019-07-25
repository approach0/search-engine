#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "common/common.h"
#include "codec/codec.h"
#include "mhook/mhook.h"

#define log2(_n) (31 - __builtin_clz(_n))

#define FIELD_INFO(_type, _member, _codec) \
	({ _type *p; ((struct field_info) { \
		offsetof(_type, _member), \
		sizeof p->_member, \
		log2(sizeof p->_member), \
		codec_new(_codec, CODEC_DEFAULT_ARGS) \
	}); })

struct field_info {
	uint offset, sz, logsz;
	struct codec *codec;
};

struct codec_buf_struct_info {
	uint              n_fields;
	struct field_info field_info[];
};

void** codec_buf_alloc(uint n, struct codec_buf_struct_info *info)
{
	void **buf = malloc(info->n_fields * sizeof(int *));

	for (int j = 0; j < info->n_fields; j++) {
		buf[j] = malloc(n * info->field_info[j].sz);
	}

	return buf;
}

void codec_buf_free(void **buf, struct codec_buf_struct_info *info)
{
	for (int j = 0; j < info->n_fields; j++) {
		free(buf[j]);
	}

	free(buf);
}

struct codec_buf_struct_info *codec_buf_struct_info_alloc(int n_fields)
{
	struct codec_buf_struct_info *p =
		malloc(sizeof *p + n_fields * sizeof(p->field_info[0]));
	p->n_fields = n_fields;
	return p;
}


void codec_buf_struct_info_free(struct codec_buf_struct_info *info)
{
	for (int j = 0; j < info->n_fields; j++) {
		codec_free(info->field_info[j].codec);
	}

	free(info);
}

void codec_buf_set(void **buf, uint i, void *item,
                   struct codec_buf_struct_info *info)
{
	for (int j = 0; j < info->n_fields; j++) {
		struct field_info f_info = info->field_info[j];
		void *p = (char *)buf[j] + (i << f_info.logsz);
		memcpy(p, (char *)item + f_info.offset, f_info.sz);
	}
}

size_t codec_buf_encode(void *dest, void **src, uint n,
                        struct codec_buf_struct_info *info)
{
	PTR_CAST(d, char, dest);

	/* write down the length */
	*(u16 *)d = n;
	d += sizeof(u16);

	/* encoding */
	for (int j = 0; j < info->n_fields; j++) {
		struct field_info f_info = info->field_info[j];
		size_t m = (n << f_info.logsz) >> log2(sizeof(uint32_t));
		d += codec_compress_ints(f_info.codec, (uint32_t*)src[j], m, d);
	}

	return (uintptr_t)((void*)d - dest);
}

size_t codec_buf_decode(void **dest, void *src, uint n,
                        struct codec_buf_struct_info *info)
{
	PTR_CAST(s, char, src);

	/* extract the length */
	u16 l = *(u16 *)s;
	s += sizeof(u16);

	/* decoding */
	for (int j = 0; j < info->n_fields; j++) {
		struct field_info f_info = info->field_info[j];
		size_t m = (l << f_info.logsz) >> 2;
		s += codec_decompress_ints(f_info.codec, s, (uint32_t*)dest[j], m);
	}

	return (uintptr_t)((void*)s - src);
}

/* test */

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
