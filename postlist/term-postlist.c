#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common/common.h"
#include "codec/codec.h"
#include "postlist-codec/postlist-codec.h"
#include "term-index/term-index.h" /* for position_t */
#include "postlist.h"
#include "config.h"

static uint32_t
onflush_for_plain_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	doc_id_t *docID = (doc_id_t *)buf;
	return *docID;
}

static void
onrebuf_for_plain_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	return;
}

static uint32_t
onflush_for_compressed_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	doc_id_t docID = *(doc_id_t *)buf;
	PTR_CAST(codec, struct postlist_codec, buf_arg);

	u32 len = (*buf_sz) / TERM_POSTLIST_ITEM_SZ;
	*buf_sz = postlist_compress(buf, buf, len, codec);
	
	return docID;
}

static void
onrebuf_for_compressed_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	u16 len = *(u16 *)buf;
	PTR_CAST(codec, struct postlist_codec, buf_arg);

	(void)postlist_decompress(buf, buf, len, codec);
	*buf_sz = len * TERM_POSTLIST_ITEM_SZ;

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
		return offsetof(struct term_posting_item, doc_id);
	case 1:
		return offsetof(struct term_posting_item, tf);
	case 2:
		return offsetof(struct term_posting_item, pos_arr);
	default:
		prerr("Unexpected field number");
		abort();
	}
}

static uint field_len(void *inst, uint j)
{
	PTR_CAST(a, struct term_posting_item, inst);
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

static uint field_size(uint j)
{
	struct term_posting_item a;
	switch (j) {
	case 0:
		return sizeof(a.doc_id);
	case 1:
		return sizeof(a.tf);
	case 2:
		return MAX_TERM_INDEX_ITEM_POSITIONS;
	default:
		prerr("Unexpected field number");
		abort();
	}
}

static char *field_info(uint j)
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

struct postlist *
term_postlist_create_plain()
{
	struct postlist *po;
	struct postlist_codec *codec;

	struct postlist_callbks calls = {
		onflush_for_plain_post,
		onrebuf_for_plain_post,
		onfree
	};
	
	codec = postlist_codec_alloc(
		TERM_POSTLIST_BUF_MAX_ITEMS,
		TERM_POSTLIST_ITEM_SZ,
		3 /* three fields */,
		field_offset, field_len,
		field_size,   field_info,
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS)
	);
	
	po = postlist_create(
		TERM_POSTLIST_SKIP_SPAN,
		TERM_POSTLIST_BUF_SZ,
		TERM_POSTLIST_ITEM_SZ,
		codec, calls);

	return po;
}

struct postlist *
term_postlist_create_compressed()
{
	struct postlist *po;
	struct postlist_codec *codec;

	struct postlist_callbks calls = {
		onflush_for_compressed_post,
		onrebuf_for_compressed_post,
		onfree
	};
	
	codec = postlist_codec_alloc(
		TERM_POSTLIST_BUF_MAX_ITEMS,
		TERM_POSTLIST_ITEM_SZ,
		3 /* three fields */,
		field_offset, field_len,
		field_size,   field_info,
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	);
	
	po = postlist_create(
		TERM_POSTLIST_SKIP_SPAN,
		TERM_POSTLIST_BUF_SZ,
		TERM_POSTLIST_ITEM_SZ,
		codec, calls);

	return po;
}
