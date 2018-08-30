#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef DEBUG_POSTLIST
#undef NDEBUG
#else
#define NDEBUG
#endif
#include <assert.h>

#include "common/common.h"
#include "codec/codec.h"
#include "postlist-codec/postlist-codec.h"
#include "postlist.h"
#include "math-postlist.h"

#ifdef DEBUG_POSTLIST
#define MATH_POSTLIST_SKIP_SPAN       2
#else
#define MATH_POSTLIST_SKIP_SPAN       DEFAULT_SKIPPY_SPANS
#endif

#define MIN_MATH_POSTING_BUF_SZ       sizeof(struct math_postlist_item)

#ifdef DEBUG_POSTLIST
#define MATH_POSTLIST_BUF_SZ          (MIN_MATH_POSTING_BUF_SZ * 2)
#else
#define MATH_POSTLIST_BUF_SZ          ROUND_UP(MIN_MATH_POSTING_BUF_SZ, 65536 * 2)
#endif

#define MATH_POSTLIST_BUF_MAX_ITEMS   \
	(MATH_POSTLIST_BUF_SZ / MIN_MATH_POSTING_BUF_SZ)

static uint64_t
onflush_for_plain_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	uint64_t compound_id = *(uint64_t *)buf;
	return compound_id;
}

static void
onrebuf_for_plain_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	return;
}

static uint64_t
onflush_for_compressed_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	uint64_t compound_id = *(uint64_t *)buf;
	PTR_CAST(codec, struct postlist_codec, buf_arg);

	u32 len = (*buf_sz) / sizeof(struct math_postlist_item);
	*buf_sz = postlist_compress(buf, buf, len, codec);
	
	return compound_id;
}

static void
onrebuf_for_compressed_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	u16 len = *(u16 *)buf;
	PTR_CAST(codec, struct postlist_codec, buf_arg);

	(void)postlist_decompress(buf, buf, len, codec);
	*buf_sz = len * sizeof(struct math_postlist_item);

	return;
}

static void
onfree(void *buf_arg)
{
	PTR_CAST(codec, struct postlist_codec, buf_arg);
	postlist_codec_free(codec);
}

static uint field_offset(uint j)
{
	switch (j) {
	case 0:
		return offsetof(struct math_postlist_item, exp_id);
	case 1:
		return offsetof(struct math_postlist_item, doc_id);
	case 2:
		return offsetof(struct math_postlist_item, n_lr_paths);
	case 3:
		return offsetof(struct math_postlist_item, n_paths);
	case 4:
		return offsetof(struct math_postlist_item, leaf_id);
	case 5:
		return offsetof(struct math_postlist_item, subr_id);
	case 6:
		return offsetof(struct math_postlist_item, lf_symb);
	default:
		prerr("Unexpected field number");
		abort();
	}
}

static uint field_len(void *inst, uint j)
{
	PTR_CAST(a, struct math_postlist_item, inst);
	switch (j) {
	case 0:
	case 1:
	case 2:
	case 3:
		return 1;
	case 4:
	case 5:
	case 6:
		return a->n_paths;
	default:
		prerr("Unexpected field number");
		abort();
	}
}

static uint field_size(uint j)
{
	size_t sz = 0;
	struct math_postlist_item a;
	switch (j) {
	case 0:
	case 1:
	case 2:
	case 3:
		sz = sizeof(uint32_t);
		break;
	case 4:
	case 5:
	case 6:
		sz = sizeof(a.leaf_id);
		break;
	default:
		prerr("Unexpected field number");
		abort();
	}

	//printf("%lu @%d\n", sz, __LINE__);
	return sz;
}

static char *field_info(uint j)
{
	switch (j) {
	case 0:
		return "expr ID";
	case 1:
		return "doc ID";
	case 2:
		return "total leaf-root paths";
	case 3:
		return "item leaf-root paths";
	case 4:
		return "path leaf-end vertex";
	case 5:
		return "path subr-end vertex";
	case 6:
		return "leaf (operand) symbol";
	default:
		prerr("Unexpected field number");
		abort();
	}
}

struct postlist *
math_postlist_create_plain()
{
	struct postlist *po;
	struct postlist_codec *codec;

	struct postlist_callbks calls = {
		onflush_for_plain_post,
		onrebuf_for_plain_post,
		onfree
	};
	
	codec = postlist_codec_alloc(
		MATH_POSTLIST_BUF_MAX_ITEMS,
		sizeof(struct math_postlist_item),
		7 /* number of fields */,
		field_offset, field_len,
		field_size,   field_info,
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS)
	);

	po = postlist_create(
		MATH_POSTLIST_SKIP_SPAN,
		MATH_POSTLIST_BUF_SZ,
		sizeof(struct math_postlist_item),
		codec, calls);

	return po;
}

struct postlist *
math_postlist_create_compressed()
{
	struct postlist *po;
	struct postlist_codec *codec;

	struct postlist_callbks calls = {
		onflush_for_compressed_post,
		onrebuf_for_compressed_post,
		onfree
	};
	
	codec = postlist_codec_alloc(
		MATH_POSTLIST_BUF_MAX_ITEMS,
		sizeof(struct math_postlist_item),
		7 /* number of fields */,
		field_offset, field_len,
		field_size,   field_info,
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	);
	
	po = postlist_create(
		MATH_POSTLIST_SKIP_SPAN,
		MATH_POSTLIST_BUF_SZ,
		sizeof(struct math_postlist_item),
		codec, calls);

	return po;
}
