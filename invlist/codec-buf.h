#pragma once
#include "common/common.h"

/* utility to calculate number of shift bits */
#define _log2_(_n) (31 - __builtin_clz(_n))

/* describe field_info by using compiler ability */
#define FIELD_INFO(_type, _member, _codec) \
	({ _type *p; ((struct field_info) { \
		offsetof(_type, _member), \
		sizeof p->_member, \
		_log2_(sizeof p->_member), \
		_log2_(MAX(1, sizeof p->_member / sizeof(int))), \
		"unset", \
		codec_new(_codec, CODEC_DEFAULT_ARGS) \
	}); })

/* get buffer item address from index */
#define CODEC_BUF_ADDR(_buf, _field, _i, _info) \
	(_buf[_field] + ((_i) << (_info)->field_info[_field].logsz))

/* retrieve item from codec buffer by memcpy */
#define CODEC_BUF_CPY(_dest, _buf, _field, _i, _info) \
	memcpy(_dest, CODEC_BUF_ADDR(_buf, _field, _i, _info), \
		(_info)->field_info[_field].sz)

/* retrieve item from codec buffer by assigning */
#define CODEC_BUF_GET(_entry, _buf, _field, _i, _info) \
	(_entry) = *(__typeof__(_entry)*) CODEC_BUF_ADDR(_buf, _field, _i, _info)

struct codec;

struct field_info {
	uint offset, sz, logsz, logints;
	char name[32];
	struct codec *codec;
};

typedef struct codec_buf_struct_info {
	uint              n_fields;
	size_t            struct_sz;
	struct field_info field_info[];
} codec_buf_struct_info_t;

void** codec_buf_alloc(uint, struct codec_buf_struct_info*);
void codec_buf_free(void **, struct codec_buf_struct_info*);

size_t codec_buf_space(uint, struct codec_buf_struct_info*);

struct codec_buf_struct_info *codec_buf_struct_info_alloc(int, size_t);
void codec_buf_struct_info_free(struct codec_buf_struct_info*);

void codec_buf_set(void **, uint, void*, struct codec_buf_struct_info*);

size_t codec_buf_encode(void*, void **, uint, struct codec_buf_struct_info*);
size_t codec_buf_decode(void **, void*, uint, struct codec_buf_struct_info*);
