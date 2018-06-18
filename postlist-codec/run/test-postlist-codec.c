#include <stdio.h>
#include <stdlib.h> /* for abort() */
#include <stddef.h> /* for offsetof() */
#include "mhook/mhook.h"
#include "common/common.h"
#include "postlist-codec.h"

struct A {
	u32 docID;
	u32 tf;
	u32 pos_arr[64];
};

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

int main()
{
	size_t sz;
	struct postlist_codec c = postlist_codec_alloc(
		5, sizeof(struct A), 3,
		test_field_offset, test_field_len,
		test_field_size, test_field_info,
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	);

	/* print the fields codec */
	postlist_print_fields(c.fields);

	/* create a test posting list */
	struct A *doc = postlist_random(5, c.fields);
	postlist_print(doc, 5, c.fields);

	/* compress the posting into a buffer */
	char encoded[256];
	sz = postlist_compress(encoded, doc, c);
	printf("%lu bytes compressed into %lu bytes. \n", 5 * sizeof(struct A), sz);

	/* decompress back to original posting list */
	char decoded[215];
	sz = postlist_decompress(decoded, encoded, c);
	printf("%lu bytes compressed data decoded. \n", sz);

	/* print the decompressed buffer */
	postlist_print(decoded, 5, c.fields);

	/* free original posting list and fields codec */
	free(doc);
	postlist_codec_free(c);

	mhook_print_unfree();
	return 0;
}
