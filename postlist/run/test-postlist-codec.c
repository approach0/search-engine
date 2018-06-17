#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mhook/mhook.h"
#include "linkli/list.h"
#include "codec/codec.h"
#include "tex-parser/vt100-color.h"

#include <string.h>

typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned int uint;

#define prerr(_fmt, ...) \
	fprintf(stderr, C_RED "Error: " C_RST _fmt "\n", ##__VA_ARGS__);

#define prinfo(_fmt, ...) \
	fprintf(stderr, C_BLUE "Info: " C_RST _fmt "\n", ##__VA_ARGS__);

struct A {
	u32 docID;
	u32 tf;
	u32 pos_arr[64];
};

uint  test_field_size(uint);
uint  test_field_offset(uint);
char *test_field_info(uint);
uint  test_field_len(void*, uint);

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

#include <stdarg.h>
struct codec **codec_new_array(int num, ...)
{
	va_list valist;
	struct codec **codec = malloc(sizeof(struct codec) * num);
	va_start(valist, num);
	for (int i = 0; i < num; i++) {
		codec[i] = va_arg(valist, struct codec *);
	}
	va_end(valist);

	return codec;
}

void codec_array_free(int num, struct codec **codec)
{
	for (int j = 0; j < num; j++) {
		codec_free(codec[j]);
	}
	free(codec);
}

struct postlist_codec
_postlist_codec_alloc(uint n, struct postlist_codec_fields fields)
{
	struct postlist_codec c;
	char *int_arr = malloc(fields.tot_size * n);
	c.array       = malloc(fields.n_fields);
	c.pos         = malloc(fields.n_fields);
	c.len         = n;
	c.fields      = fields;

	uint offset = 0;
	for (uint j = 0; j < fields.n_fields; j++) {
		c.array[j] = (u32 *)(int_arr + offset);
		c.pos[j] = 0;
		offset += n * fields.size(j);
	}

	return c;
}

void postlist_codec_free(struct postlist_codec c)
{
	free(c.array[0]); /* free entire encode buffer */
	free(c.array); /* free array pointers */
	free(c.pos); /* free array positions */

	/* now free fields codec array */
	codec_array_free(c.fields.n_fields, c.fields.field_codec);
}

void
postlist_print_fields(struct postlist_codec_fields fields)
{
	for (uint j = 0; j < fields.n_fields; j++) {
		prinfo("Field '%s'", fields.info(j));
		prinfo("offset: %u", fields.offset(j));
		prinfo("size: %u", fields.size(j));
		if (fields.field_codec) {
			enum codec_method m = fields.field_codec[j]->method;
			prinfo("codec: %s", codec_method_str(m));
		}
	}
}

void*
postlist_random(uint n, struct postlist_codec_fields fields)
{
	void *po = malloc(n * fields.tot_size);
	srand(time(0));
	for (uint i = 0; i < n; i++) {
		void *cur = (char *)po + i * fields.tot_size;
#ifdef DEBUG_POSTLIST_CODEC
		printf("doc[%u]: \n", i);
#endif
		for (uint j = 0; j < fields.n_fields; j++) {
			uint *p = (uint *)((char *)cur + fields.offset(j));
			uint len = fields.len(cur, j);
			char *info = fields.info(j);
			for (uint k = 0; k < len; k++) {
				if (k == 0) {
					if (i == 0 || strstr(info, "ID") == NULL) {
						p[0] = rand() % 10 + 1;
					} else {
						uint last = *(uint *)((char *)po +
						             (i - 1) * fields.tot_size +
						             fields.offset(j));
						// printf("last = %u \n", last);
						p[0] = last + rand() % 10;
					}
				} else {
					p[k] = p[k - 1] + rand() % 10;
				}
#ifdef DEBUG_POSTLIST_CODEC
				printf("field[%u][%u] = %u \n", j, k, p[k]);
#endif
			}
		}
	}

	return po;
}

void
postlist_print(void *po, uint n, struct postlist_codec_fields fields)
{
	for (uint i = 0; i < n; i++) {
		void *cur = (char *)po + i * fields.tot_size;
		for (uint j = 0; j < fields.n_fields; j++) {
			uint *p = (uint *)((char *)cur + fields.offset(j));
			uint len = fields.len(cur, j);
			printf("[");
			for (uint k = 0; k < len; k++) {
				printf("%u", p[k]);
				if (k + 1 != len) {
					printf(", ");
				}
			}
			printf("]");
		}
		printf("\n");
	}
}

static void _print_codec_arrays(struct postlist_codec c)
{
	for (uint j = 0; j < c.fields.n_fields; j++) {
		for (uint i = 0; i < c.pos[j]; i++)
			printf("%u ", c.array[j][i]);
		printf("\n");
	}
}

size_t
postlist_compress(void *dest, void *src, struct postlist_codec c)
{
	for (uint i = 0; i < c.len; i++) {
		void *cur = (char *)src + i * c.fields.tot_size;
		for (uint j = 0; j < c.fields.n_fields; j++) {
			uint *p = (uint *)((char *)cur + c.fields.offset(j));
			uint len = c.fields.len(cur, j);
			memcpy(c.array[j] + c.pos[j], p, len * sizeof(u32));
			c.pos[j] += len;
		}
	}
#ifdef DEBUG_POSTLIST_CODEC
	_print_codec_arrays(c);
#endif
	char *d = (char *)dest;
	for (uint j = 0; j < c.fields.n_fields; j++) {
		struct codec *codec = c.fields.field_codec[j];
		/* write length info */
		*(u16 *)d = (u16)c.pos[j];
		d += sizeof(u16);
		/* write compressed data */
		d += codec_compress_ints(codec, c.array[j], c.pos[j], d);
		c.pos[j] = 0; /* reset position */
	}

	return (size_t)(d - (char*)dest);
}

size_t
postlist_decompress(void *dest, void *src, struct postlist_codec c)
{
	char *s = (char *)src;
	for (uint j = 0; j < c.fields.n_fields; j++) {
		struct codec *codec = c.fields.field_codec[j];
		/* read length info */
		u16 l = *(u16 *)s;
		s += sizeof(u16);
		/* write decompressed data */
		s += codec_decompress_ints(codec, s, c.array[j], l);
		c.pos[j] = 0; /* reset position */
	}

	for (uint i = 0; i < c.len; i++) {
		void *cur = (char *)dest + i * c.fields.tot_size;
		for (uint j = 0; j < c.fields.n_fields; j++) {
			uint *p = (uint *)((char *)cur + c.fields.offset(j));
			uint len = c.fields.len(cur, j);
			memcpy(p, c.array[j] + c.pos[j], len * sizeof(u32));
			c.pos[j] += len;
		}
	}
#ifdef DEBUG_POSTLIST_CODEC
	_print_codec_arrays(c);
#endif
	return (size_t)(s - (char*)src);
}

#define postlist_codec_alloc(_alloc_space, _tot_size, _n_fields, \
	_field_offset, _field_len, _field_size, _field_info, ...) \
	_postlist_codec_alloc(_alloc_space, (struct postlist_codec_fields) { \
		_tot_size, _n_fields, _field_offset, _field_len, _field_size, \
		_field_info, codec_new_array(_n_fields, __VA_ARGS__) \
	});


int main()
{
	struct postlist_codec c = postlist_codec_alloc(
		5, sizeof(struct A), 3,
		test_field_offset, test_field_len,
		test_field_size, test_field_info,
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	);
	size_t sz;

	postlist_print_fields(c.fields);

	struct A *doc = postlist_random(5, c.fields);
	postlist_print(doc, 5, c.fields);
	char encoded[256];
	sz = postlist_compress(encoded, doc, c);
	printf("%lu bytes compressed into %lu bytes. \n", 5 * sizeof(struct A), sz);
	char decoded[215];
	sz = postlist_decompress(decoded, encoded, c);
	printf("%lu bytes compressed data decoded. \n", sz);
	postlist_print(decoded, 5, c.fields);
	free(doc);

	postlist_codec_free(c);
	mhook_print_unfree();
	return 0;
}

uint test_field_offset(uint j)
{
	switch (j) {
	case 0:
		return offsetof(struct A, docID);
	case 1:
		return offsetof(struct A, tf);
	case 2:
		return offsetof(struct A, pos_arr);
	default:
		prerr("Unexpected field number");
		abort();
	}
}

uint test_field_len(void *inst, uint j)
{
	struct A *a = (struct A*)inst;
	switch (j) {
	case 0:
		return 1;
	case 1:
		return 1;
	case 2:
		return a->tf;
	default:
		prerr("Unexpected field number");
		abort();
	}
}

uint test_field_size(uint j)
{
	struct A a;
	switch (j) {
	case 0:
		return sizeof(a.docID);
	case 1:
		return sizeof(a.tf);
	case 2:
		return sizeof(a.pos_arr);
	default:
		prerr("Unexpected field number");
		abort();
	}
}

char *test_field_info(uint j)
{
	switch (j) {
	case 0:
		return "doc ID";
	case 1:
		return "term freq";
	case 2:
		return "positions";
	default:
		prerr("Unexpected field number");
		abort();
	}
}
