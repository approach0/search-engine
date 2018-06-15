#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "mhook/mhook.h"
#include "linkli/list.h"
#include "codec/codec.h"
#include "tex-parser/vt100-color.h"

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

struct postlist_codec_args {
	uint           size;
	uint           n_fields;
	uint          (*field_offset)(uint);
	uint          (*field_len)(void*, uint);
	uint          (*field_size)(uint);
	char         *(*field_info)(uint);
	struct codec *(*field_codec)(uint);
};

struct postlist_codec {
	u32  **array; /* temporary ints array */
	uint  *pos;   /* array current positions */
	struct postlist_codec_args args;
};

struct postlist_codec
postlist_codec_alloc(uint n, struct postlist_codec_args args)
{
	struct postlist_codec c;
	char *int_arr = malloc(args.size * n);
	c.pos         = malloc(args.n_fields);
	c.array       = malloc(args.n_fields);
	c.args        = args;

	uint offset = 0;
	for (uint j = 0; j < args.n_fields; j++) {
		c.array[j] = (u32 *)(int_arr + offset);
		c.pos[j] = 0;
		offset += n * args.field_size(j);
	}

	return c;
}

void postlist_codec_free(struct postlist_codec *c)
{
	free(c->array[0]); /* free entire encode buffer */
	free(c->array); /* free array pointers */
	free(c->pos); /* free array positions */
}

void
postlist_print_fields(struct postlist_codec_args args)
{
	for (uint j = 0; j < args.n_fields; j++) {
		prinfo("Field [%s]", args.field_info(j));
		prinfo("offset: %u", args.field_offset(j));
		prinfo("size: %u", args.field_size(j));
	}
}

void*
postlist_random(uint n, struct postlist_codec_args args)
{
	void *po = malloc(n * args.size);
	srand(time(0));
	for (uint i = 0; i < n; i++) {
		void *cur = (char *)po + i * args.size;
		// printf("doc[%u]: \n", i);
		for (uint j = 0; j < args.n_fields; j++) {
			uint *p = (uint *)((char *)cur + args.field_offset(j));
			uint field_len = args.field_len(cur, j);
			for (uint k = 0; k < field_len; k++) {
				p[k] = rand() % 10 + 1;
				// printf("field[%u][%u] = %u \n", j, k, p[k]);
			}
		}
	}

	return po;
}

void
postlist_print(void *po, uint n, struct postlist_codec_args args)
{
	for (uint i = 0; i < n; i++) {
		void *cur = (char *)po + i * args.size;
		for (uint j = 0; j < args.n_fields; j++) {
			uint *p = (uint *)((char *)cur + args.field_offset(j));
			uint field_len = args.field_len(cur, j);
			printf("[");
			for (uint k = 0; k < field_len; k++) {
				printf("%u", p[k]);
				if (k + 1 != field_len) {
					printf(", ");
				}
			}
			printf("]");
		}
		printf("\n");
	}
}

int main()
{
	struct postlist_codec_args args = {
		sizeof(struct A), 3, test_field_offset, test_field_len,
		test_field_size, test_field_info, NULL
	};

	postlist_print_fields(args);

	struct A *doc = postlist_random(5, args);
	postlist_print(doc, 5, args);
	free(doc);

	struct postlist_codec c = postlist_codec_alloc(5, args);
	postlist_codec_free(&c);

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
