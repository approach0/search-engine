#include "common/common.h"
#include "codec/codec.h" /* for codec_new_array */

struct postlist_codec_fields {
	uint           tot_size;
	uint           n_fields;
	uint          (*offset)(uint);
	uint          (*len)(void*, uint);
	uint          (*size)(uint);
	char         *(*info)(uint);
	struct codec **field_codec;
};

struct postlist_codec {
	u32  **array; /* temporary ints array */
	uint  *pos;   /* array current positions */
	uint   len;   /* array total length */
	struct postlist_codec_fields fields;
};

/* postlist_codec  constructor macro */
#define postlist_codec_alloc(_alloc_space, _tot_size, _n_fields, \
	_field_offset, _field_len, _field_size, _field_info, ...) \
	_postlist_codec_alloc(_alloc_space, (struct postlist_codec_fields) { \
		_tot_size, _n_fields, _field_offset, _field_len, _field_size, \
		_field_info, codec_new_array(_n_fields, __VA_ARGS__) \
	});

struct postlist_codec
_postlist_codec_alloc(uint, struct postlist_codec_fields);

void postlist_codec_free(struct postlist_codec);

void postlist_print_fields(struct postlist_codec_fields);

void* postlist_random(uint, struct postlist_codec_fields);

void postlist_print(void*, uint, struct postlist_codec_fields);

size_t postlist_compress(void*, void*, struct postlist_codec);

size_t postlist_decompress(void*, void*, struct postlist_codec);
