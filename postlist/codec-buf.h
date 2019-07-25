#pragma once
#include "common/common.h"

#define log2(_n) (31 - __builtin_clz(_n))

#define FIELD_INFO(_type, _member, _codec) \
	({ _type *p; ((struct field_info) { \
		offsetof(_type, _member), \
		sizeof p->_member, \
		log2(sizeof p->_member), \
		codec_new(_codec, CODEC_DEFAULT_ARGS) \
	}); })

#define CODEC_BUF_GET(_entry, _buf, _field, _i, _info) \
	(_entry) = *(__typeof__(_entry)*)(_buf[_field] + (_i << (_info)->field_info[_field].logsz));

struct codec;

struct field_info {
	uint offset, sz, logsz;
	struct codec *codec;
};

struct codec_buf_struct_info {
	uint              n_fields;
	struct field_info field_info[];
};

void** codec_buf_alloc(uint, struct codec_buf_struct_info*);
void codec_buf_free(void **, struct codec_buf_struct_info*);

struct codec_buf_struct_info *codec_buf_struct_info_alloc(int);
void codec_buf_struct_info_free(struct codec_buf_struct_info*);

void codec_buf_set(void **, uint, void*, struct codec_buf_struct_info*);

size_t codec_buf_encode(void*, void **, uint, struct codec_buf_struct_info*);
size_t codec_buf_decode(void **, void*, uint, struct codec_buf_struct_info*);
