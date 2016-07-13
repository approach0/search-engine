#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term-index/term-index.h" /* for position_t */
#include "codec/codec.h"
#include "mem-posting-calls.h"
#include "config.h"

/* include assert */
#ifdef DEBUG_MEM_POSTING
#undef N_DEBUG
#else
#define N_DEBUG
#endif
#include <assert.h>

char *getposarr_for_termpost(char *buf, size_t *size)
{
	uint32_t *tf = (uint32_t *)buf + 1;
	*size = 0;
	return (char *)(tf + 1);
}

char *getposarr_for_termpost_with_pos(char *buf, size_t *size)
{
	uint32_t *tf = (uint32_t *)buf + 1;
	*size = sizeof(position_t) * (*tf);
	return (char *)(tf + 1);
}

uint32_t onflush_for_plainpost(char *buf, uint32_t *buf_sz)
{
	doc_id_t *docID = (doc_id_t *)buf;
	return *docID;
}

void onrebuf_for_plainpost(char *buf, uint32_t *buf_sz)
{
	return;
}

static size_t extract_docid_tf(char *cur, doc_id_t *docID, uint32_t *tf)
{
	/* extract values */
	*docID = *(doc_id_t *)cur;
	*tf = *(uint32_t *)(cur + sizeof(doc_id_t));

	/* return cur pointer increment */
	return (sizeof(doc_id_t) + sizeof(uint32_t));
}

static size_t extract_pos_arr(char *cur, uint32_t tf,
                              position_t *pos_arr, uint32_t *pos_idx)
{
	size_t pos_sz = tf * sizeof(position_t);
	memcpy(pos_arr + (*pos_idx), cur, pos_sz);
	*pos_idx += tf;
	return pos_sz;
}

uint32_t onflush_for_termpost(char *buf, uint32_t *buf_sz)
{
	/* save key ID to be returned */
	doc_id_t save_key = *(doc_id_t *)buf;

	/* parser pointers */
	char    *cur = buf;
	uint32_t cur_offset = 0;
	uint32_t i;

	/* intermediate arrays */
	doc_id_t docID_arr[MEM_POSTING_BUF_SZ];
	uint32_t tf_arr[MEM_POSTING_BUF_SZ];

	/* codecs */
	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	/*
	 * parse linear buffer into different intermediate arrays.
	 */
	for (i = 0; cur_offset != *buf_sz; i++) {
		cur += extract_docid_tf(cur, docID_arr + i, tf_arr + i);

		cur_offset = (uint32_t)(cur - buf);
		assert(cur_offset <= *buf_sz);
	}

	/*
	 * for each intermediate array, compress and output back to buffer.
	 */
	{
		size_t size, now = 0;
		uint32_t *head = (uint32_t *)buf;
		char *payload  = (char *)(head + 1);
		*head = i;

		size = codec_compress_ints(codec[0], docID_arr, i, payload + now);
		now += size;
#ifdef DEBUG_MEM_POSTING_SHOW_COMPRESS_RATE
		printf("codec[0]: %u ==> %lu (%.3f).\n", i << 2, size,
		       (float)(i << 2) / (float)size);
#endif

		size = codec_compress_ints(codec[1], tf_arr, i, payload + now);
		now += size;
#ifdef DEBUG_MEM_POSTING_SHOW_COMPRESS_RATE
		printf("codec[1]: %u ==> %lu (%.3f).\n", i << 2, size,
		       (float)(i << 2) / (float)size);
#endif

		assert(now <= MEM_POSTING_BUF_SZ);
		*buf_sz = sizeof(uint32_t) /* header size */ + now /* payload size */;
	}

	/* free codecs allocated */
	free(codec[0]);
	free(codec[1]);

	return save_key;
}

uint32_t onflush_for_termpost_with_pos(char *buf, uint32_t *buf_sz)
{
	/* save key ID to be returned */
	doc_id_t save_key = *(doc_id_t *)buf;

	/* parser pointers */
	char    *cur = buf;
	uint32_t cur_offset = 0;
	uint32_t i;

	/* intermediate arrays */
	doc_id_t  docID_arr[MEM_POSTING_BUF_SZ];
	uint32_t  tf_arr[MEM_POSTING_BUF_SZ];
	position_t pos_arr[MEM_POSTING_BUF_SZ];
	uint32_t   pos_idx = 0;

	/* codecs */
	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	/*
	 * parse linear buffer into different intermediate arrays.
	 */
	for (i = 0; cur_offset != *buf_sz; i++) {
		cur += extract_docid_tf(cur, docID_arr + i, tf_arr + i);

		/* also extract position array */
		cur += extract_pos_arr(cur, tf_arr[i], pos_arr, &pos_idx);

		cur_offset = (uint32_t)(cur - buf);
		assert(cur_offset <= *buf_sz);
	}

	/*
	 * for each intermediate array, compress and output back to buffer.
	 */
	{
		size_t size, now = 0;
		uint32_t *head = (uint32_t *)buf;
		char *payload  = (char *)(head + 1);
		*head = i;

		size = codec_compress_ints(codec[0], docID_arr, i, payload + now);
		now += size;
#ifdef DEBUG_MEM_POSTING_SHOW_COMPRESS_RATE
		printf("codec[0]: %u ==> %lu (%.3f).\n", i << 2, size,
		       (float)(i << 2) / (float)size);
#endif

		size = codec_compress_ints(codec[1], tf_arr, i, payload + now);
		now += size;
#ifdef DEBUG_MEM_POSTING_SHOW_COMPRESS_RATE
		printf("codec[1]: %u ==> %lu (%.3f).\n", i << 2, size,
		       (float)(i << 2) / (float)size);
#endif

		size = codec_compress_ints(codec[2], pos_arr, pos_idx, payload + now);
		now += size;
#ifdef DEBUG_MEM_POSTING_SHOW_COMPRESS_RATE
		printf("codec[2]: %u ==> %lu (%.3f).\n", pos_idx << 2, size,
		       (float)(pos_idx << 2) / (float)size);
#endif

		assert(now <= MEM_POSTING_BUF_SZ);
		*buf_sz = sizeof(uint32_t) /* header size */ + now /* payload size */;
	}

	/* free codecs allocated */
	free(codec[0]);
	free(codec[1]);
	free(codec[2]);

	return save_key;
}

static size_t restore_docid_tf(char *cur, doc_id_t docID, uint32_t tf)
{
	/* restore values */
	*(doc_id_t *)cur = docID;
	*(uint32_t *)(cur + sizeof(doc_id_t)) = tf;

	/* return cur pointer increment */
	return (sizeof(doc_id_t) + sizeof(uint32_t));
}

static size_t restore_pos_arr(char *cur, uint32_t tf,
                              position_t *pos_arr, uint32_t *pos_idx)
{
	size_t pos_sz = tf * sizeof(position_t);
	memcpy(cur, pos_arr + (*pos_idx), pos_sz);
	*pos_idx += tf;
	return pos_sz;
}

void onrebuf_for_termpost(char *buf, uint32_t *buf_sz)
{
	/* get parser pointers */
	uint32_t *head = (uint32_t *)buf;
	uint32_t i, n = *head;
	char *cur;

	/* intermediate arrays */
	doc_id_t docID_arr[MEM_POSTING_BUF_SZ];
	uint32_t tf_arr[MEM_POSTING_BUF_SZ];

	/* codecs */
	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	/*
	 * decompress payload to intermediate arrays
	 */
	cur = (char *)(head + 1);
	cur += codec_decompress_ints(codec[0], cur, docID_arr, n);
	cur += codec_decompress_ints(codec[1], cur, tf_arr, n);

	/*
	 * restore values back to buffer.
	 */
	cur = buf;
	for (i = 0; i < n; i++) {
		cur += restore_docid_tf(cur, docID_arr[i], tf_arr[i]);
	}

	/* output new buffer size */
	{
		uint32_t offset = (uint32_t)(cur - buf);
		assert(offset <= MEM_POSTING_BUF_SZ);
		*buf_sz = offset;
	}

	/* free codecs allocated */
	free(codec[0]);
	free(codec[1]);

	return;
}

void onrebuf_for_termpost_with_pos(char *buf, uint32_t *buf_sz)
{
	/* get parser pointers */
	uint32_t *head = (uint32_t *)buf;
	uint32_t i, n = *head;
	char *cur;

	/* intermediate arrays */
	doc_id_t docID_arr[MEM_POSTING_BUF_SZ];
	uint32_t tf_arr[MEM_POSTING_BUF_SZ];
	position_t pos_arr[MEM_POSTING_BUF_SZ];
	uint32_t   pos_idx = 0;

	/* codecs */
	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	/*
	 * decompress payload to intermediate arrays
	 */
	cur = (char *)(head + 1);
	cur += codec_decompress_ints(codec[0], cur, docID_arr, n);
	cur += codec_decompress_ints(codec[1], cur, tf_arr, n);

	/* also decompress positions */
	for (i = 0; i < n; i++)
		pos_idx += tf_arr[i];

	cur += codec_decompress_ints(codec[2], cur, pos_arr, pos_idx);

	/*
	 * restore values back to buffer.
	 */
	cur = buf;
	pos_idx = 0;
	for (i = 0; i < n; i++) {
		cur += restore_docid_tf(cur, docID_arr[i], tf_arr[i]);
		cur += restore_pos_arr(cur, tf_arr[i], pos_arr, &pos_idx);
	}

	/* output new buffer size */
	{
		uint32_t offset = (uint32_t)(cur - buf);
		assert(offset <= MEM_POSTING_BUF_SZ);
		*buf_sz = offset;
	}

	/* free codecs allocated */
	free(codec[0]);
	free(codec[1]);
	free(codec[2]);

	return;
}
