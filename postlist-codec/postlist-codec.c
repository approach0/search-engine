#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h> /* for strstr */

#include "codec/codec.h"
#include "postlist-codec.h"

struct postlist_codec *
_postlist_codec_alloc(uint n, struct postlist_codec_fields fields)
{
	struct postlist_codec *c = malloc(sizeof(struct postlist_codec));
	char *int_arr  = malloc(fields.tot_size * n);
	c->array       = malloc(sizeof(u32*) * fields.n_fields);
	c->pos         = malloc(sizeof(uint) * fields.n_fields);
	c->fields      = fields;

	uint offset = 0;
	for (uint j = 0; j < fields.n_fields; j++) {
		c->array[j] = (u32 *)(int_arr + offset);
		c->pos[j] = 0;
		offset += n * fields.size(j);
	}

	return c;
}

void postlist_codec_free(struct postlist_codec *c)
{
	free(c->array[0]); /* free entire encode buffer */
	free(c->array); /* free array pointers */
	free(c->pos); /* free array positions */

	/* now free fields codec array */
	codec_array_free(c->fields.n_fields, c->fields.field_codec);

	/* free the structure itself */
	free(c);
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

static void _print_codec_arrays(struct postlist_codec *c)
{
	for (uint j = 0; j < c->fields.n_fields; j++) {
		for (uint i = 0; i < c->pos[j]; i++)
			printf("%u ", c->array[j][i]);
		printf("\n");
	}
}

size_t
postlist_compress(void *dest, void *src, uint n, struct postlist_codec *c)
{
	/* separate values by fields, put into corresponding field array */
	for (uint i = 0; i < n; i++) {
		void *cur = (char *)src + i * c->fields.tot_size;
		for (uint j = 0; j < c->fields.n_fields; j++) {
			uint *p = (uint *)((char *)cur + c->fields.offset(j));
			uint len = c->fields.len(cur, j);
			memcpy(c->array[j] + c->pos[j], p, len * sizeof(u32));
			c->pos[j] += len;
		}
	}
#ifdef DEBUG_POSTLIST_CODEC
	_print_codec_arrays(c);
#endif
	char *d = (char *)dest;
	for (uint j = 0; j < c->fields.n_fields; j++) {
		struct codec *codec = c->fields.field_codec[j];
		/* write length info */
		*(u16 *)d = (u16)c->pos[j];
		d += sizeof(u16);
		/* write compressed data */
		d += codec_compress_ints(codec, c->array[j], c->pos[j], d);
		c->pos[j] = 0; /* reset position */
	}

	return (size_t)(d - (char*)dest);
}

size_t
postlist_decompress(void *dest, void *src, uint n, struct postlist_codec *c)
{
	char *s = (char *)src;
	for (uint j = 0; j < c->fields.n_fields; j++) {
		struct codec *codec = c->fields.field_codec[j];
		/* read length info */
		u16 l = *(u16 *)s;
		s += sizeof(u16);
		/* write decompressed data */
		s += codec_decompress_ints(codec, s, c->array[j], l);
		c->pos[j] = 0; /* reset position */
	}

	for (uint i = 0; i < n; i++) {
		void *cur = (char *)dest + i * c->fields.tot_size;
		for (uint j = 0; j < c->fields.n_fields; j++) {
			uint *p = (uint *)((char *)cur + c->fields.offset(j));
			uint len = c->fields.len(cur, j);
			memcpy(p, c->array[j] + c->pos[j], len * sizeof(u32));
			c->pos[j] += len;
		}
	}
#ifdef DEBUG_POSTLIST_CODEC
	_print_codec_arrays(c);
#endif
	return (size_t)(s - (char*)src);
}
