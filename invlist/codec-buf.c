#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codec/codec.h"
#include "codec-buf.h"

void** codec_buf_alloc(uint n, struct codec_buf_struct_info *info)
{
	void **buf = malloc(info->n_fields * sizeof(int *));

	for (int j = 0; j < info->n_fields; j++) {
		buf[j] = malloc(n * info->field_info[j].sz);
	}

	return buf;
}

size_t codec_buf_space(uint n, struct codec_buf_struct_info *info)
{
	size_t tot_sz = info->n_fields * sizeof(int *);

	for (int j = 0; j < info->n_fields; j++) {
		tot_sz += n * info->field_info[j].sz;
	}

	return tot_sz;
}

void codec_buf_free(void **buf, struct codec_buf_struct_info *info)
{
	for (int j = 0; j < info->n_fields; j++) {
		free(buf[j]);
	}

	free(buf);
}

struct codec_buf_struct_info
*codec_buf_struct_info_alloc(int n_fields, size_t size)
{
	struct codec_buf_struct_info *p =
		malloc(sizeof *p + n_fields * sizeof(p->field_info[0]));
	p->n_fields = n_fields;
	p->align_sz = size;
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
		struct field_info *f_info = info->field_info + j;
		n = n << f_info->logints;
		d += codec_compress_ints(f_info->codec, (uint32_t*)src[j], n, d);
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
		struct field_info *f_info = info->field_info + j;
		l = l << f_info->logints;
		s += codec_decompress_ints(f_info->codec, s, (uint32_t*)dest[j], l);
	}

	return (uintptr_t)((void*)s - src);
}
