#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include <assert.h>

#include "common/common.h"
#include "codec/codec.h"
#include "postlist-codec/postlist-codec.h"
#include "postlist.h"
#include "math-postlist.h"

/* skip list span */
#define MATH_POSTLIST_SKIP_SPAN   DEFAULT_SKIPPY_SPANS

/* buffer sizes */
#ifdef DEBUG_POSTLIST
#define BUF_SZ(_item_struct)  (sizeof(_item_struct) * 2)
#else
//#define BUF_SZ(_item_struct)  ROUND_UP(65536, sizeof(_item_struct))
#define BUF_SZ(_item_struct)  ROUND_UP(32768, sizeof(_item_struct))
#endif

#define BUF_MAX_ITEMS(_item_struct)   \
	(BUF_SZ(_item_struct) / sizeof(_item_struct))

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

static uint64_t
onflush_for_compressed_gener_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	uint64_t compound_id = *(uint64_t *)buf;
	PTR_CAST(codec, struct postlist_codec, buf_arg);

	u32 len = (*buf_sz) / sizeof(struct math_postlist_gener_item);
	*buf_sz = postlist_compress(buf, buf, len, codec);

	return compound_id;
}

static void
onrebuf_for_compressed_gener_post(char *buf, uint32_t *buf_sz, void *buf_arg)
{
	u16 len = *(u16 *)buf;
	PTR_CAST(codec, struct postlist_codec, buf_arg);

	(void)postlist_decompress(buf, buf, len, codec);
	*buf_sz = len * sizeof(struct math_postlist_gener_item);

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
		return offsetof(struct math_postlist_gener_item, exp_id);
	case 1:
		return offsetof(struct math_postlist_gener_item, doc_id);
	case 2:
		return offsetof(struct math_postlist_gener_item, n_lr_paths);
	case 3:
		return offsetof(struct math_postlist_gener_item, n_paths);
	case 4:
		return offsetof(struct math_postlist_gener_item, subr_id);
	case 5:
		return offsetof(struct math_postlist_gener_item, op_hash);
	case 6:
		return offsetof(struct math_postlist_gener_item, tr_hash);
	case 7:
		return offsetof(struct math_postlist_gener_item, wild_leaves);
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
	case 7:
		return a->n_paths;
	default:
		prerr("Unexpected field number");
		abort();
	}
}

static uint field_len_gener(void *inst, uint j)
{
	PTR_CAST(a, struct math_postlist_gener_item, inst);
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
	case 7:
		return (a->n_paths << 1);
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
	case 7:
		sz = sizeof(a.subr_id);
		break;
	default:
		prerr("Unexpected field number");
		abort();
	}

	//printf("%lu @%d\n", sz, __LINE__);
	return sz;
}

static uint field_size_gener(uint j)
{
	size_t sz = 0;
	struct math_postlist_gener_item a;
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
		sz = sizeof(a.subr_id);
		break;
	case 7:
		sz = sizeof(a.wild_leaves);
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
		return "path subr-end vertex";
	case 5:
		return "operators symbol hash";
	case 6:
		return "operand(s) symbol/hash";
	case 7:
		return "operands ID(s)";
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
		BUF_MAX_ITEMS(struct math_postlist_item),
		sizeof(struct math_postlist_item),
		8 /* number of fields */,
		field_offset, field_len,
		field_size,   field_info,
		/* 0 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 1 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 2 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 3 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 4 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 5 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 6 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 7 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS)
	);

	po = postlist_create(
		MATH_POSTLIST_SKIP_SPAN,
		BUF_SZ(struct math_postlist_item),
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
		BUF_MAX_ITEMS(struct math_postlist_item),
		sizeof(struct math_postlist_item),
		8 /* number of fields */,
		field_offset, field_len,
		field_size,   field_info,
		/* 0 */ codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		/* 1 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 2 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 3 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 4 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 5 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 6 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 7 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	);

	po = postlist_create(
		MATH_POSTLIST_SKIP_SPAN,
		BUF_SZ(struct math_postlist_item),
		sizeof(struct math_postlist_item),
		codec, calls);

	return po;
}

struct postlist *
math_postlist_create_gener_plain()
{
	struct postlist *po;
	struct postlist_codec *codec;

	struct postlist_callbks calls = {
		onflush_for_plain_post,
		onrebuf_for_plain_post,
		onfree
	};

	codec = postlist_codec_alloc(
		BUF_MAX_ITEMS(struct math_postlist_gener_item),
		sizeof(struct math_postlist_gener_item),
		8 /* number of fields */,
		field_offset, field_len_gener,
		field_size_gener,   field_info,
		/* 0 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 1 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 2 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 3 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 4 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 5 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 6 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS),
		/* 7 */ codec_new(CODEC_PLAIN, CODEC_DEFAULT_ARGS)
	);

	po = postlist_create(
		MATH_POSTLIST_SKIP_SPAN,
		BUF_SZ(struct math_postlist_gener_item),
		sizeof(struct math_postlist_gener_item),
		codec, calls);

	return po;
}

struct postlist *
math_postlist_create_gener_compressed()
{
	struct postlist *po;
	struct postlist_codec *codec;

	struct postlist_callbks calls = {
		onflush_for_compressed_gener_post,
		onrebuf_for_compressed_gener_post,
		onfree
	};

	codec = postlist_codec_alloc(
		BUF_MAX_ITEMS(struct math_postlist_gener_item),
		sizeof(struct math_postlist_gener_item),
		8 /* number of fields */,
		field_offset, field_len_gener,
		field_size_gener,   field_info,
		/* 0 */ codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		/* 1 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 2 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 3 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 4 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 5 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 6 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		/* 7 */ codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	);

	po = postlist_create(
		MATH_POSTLIST_SKIP_SPAN,
		BUF_SZ(struct math_postlist_gener_item),
		sizeof(struct math_postlist_gener_item),
		codec, calls);

	return po;
}

/* print functions */
void math_postlist_print_item(struct math_postlist_gener_item *item,
                              int gener, trans_fun_t trans)
{
	printf("doc#%u, exp#%u, ", item->doc_id, item->exp_id);
	printf("lr#%u, %u paths:\n", item->n_lr_paths, item->n_paths);

	for (int i = 0; i < item->n_paths; i++) {
		if (gener) {
			printf("\t gener pathinfo %u, 0x%x, 0x%x, 0x%lx \n",
				item->subr_id[i],
				item->op_hash[i],
				item->tr_hash[i],
				item->wild_leaves[i]
			);
		} else {
			PTR_CAST(_item, struct math_postlist_item, item);
			if (trans == NULL)
				printf("\t normal pathinfo %u, 0x%x, 0x%x, %u \n",
					_item->subr_id[i],
					_item->op_hash[i],
					_item->lf_symb[i],
					_item->leaf_id[i]
				);
			else
				printf("\t normal pathinfo %u, 0x%x, %s, %u \n",
					_item->subr_id[i],
					_item->op_hash[i],
					trans(_item->lf_symb[i]),
					_item->leaf_id[i]
				);
		}
	}
}
