#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#include "config.h"
#include <assert.h>

#include "common/common.h"
#include "invlist.h"

struct invlist
*invlist_open(const char *path, uint32_t n, codec_buf_struct_info_t* c_info)
{
	struct invlist *ret = malloc(sizeof *ret);

	if (path == NULL) {
		ret->head = NULL;
		ret->type = INVLIST_TYPE_INMEMO;
	} else {
		strcpy(ret->path, path);
		ret->type = INVLIST_TYPE_ONDISK;
	}

	ret->tot_sz = sizeof *ret;
	ret->n_blk = 0;
	skippy_init(&ret->skippy, n);
	ret->buf_max_len = n;
	ret->buf_max_sz = n * c_info->align_sz;
	ret->c_info = c_info;

	return ret;
}

void invlist_free(struct invlist *inv)
{
	skippy_free(&inv->skippy, struct invlist_node, sn, free(p->blk); free(p));
	free(inv);
}

static struct invlist_node
*create_node(uint64_t key, uint16_t size, uint16_t len)
{
	struct invlist_node *ret = malloc(sizeof *ret);

	skippy_node_init(&ret->sn, key);
	ret->blk = malloc(size);
	ret->len = len;
	ret->size = size;

	return ret;
}

static void append_node(struct invlist *invlist,
	struct invlist_node *node, size_t node_sz)
{
	if (invlist->head == NULL)
		invlist->head = node;

	invlist->tot_sz += node_sz;
	invlist->n_blk ++;
	skippy_append(&invlist->skippy, &node->sn);
}

static struct skippy_data
ondisk_invlist_block_writer(struct skippy_node *blk_, void *args_)
{
	struct skippy_data sd;
	struct invlist_node *node;

	/* get node pointer address */
	node = container_of(blk_, struct invlist_node, sn);
	sd.key = node->sn.key; /* return key */

	/* get other arguments */
	PTR_CAST(fh, FILE, args_);

	/* record the current file offset */
	fseek(fh, 0, SEEK_END);
	sd.child_offset = ftell(fh);

	/* write the invlist_node onto disk */
	fwrite(&node->len, 1, sizeof node->len, fh);
	fwrite(&node->size, 1, sizeof node->size, fh);
	fwrite(node->blk, 1, node->size, fh);

	return sd;
}

size_t invlist_writer_flush(struct invlist_iterator *iter)
{
	struct invlist *invlist = iter->invlist;
	size_t payload_sz;

	/* encode current buffer integers */
	char enc_buf[invlist->buf_max_sz];
	payload_sz = codec_buf_encode(enc_buf,
		iter->buf, iter->buf_len, iter->c_info);

	/* get the key for this flushed block */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	uint32_t key_[2];
	/* assume the order is little-indian */
	CODEC_BUF_GET(key_[1], iter->buf, 0, 0, iter->c_info);
	CODEC_BUF_GET(key_[0], iter->buf, 1, 0, iter->c_info);
	uint64_t key = *(uint64_t*)key_;
#else
#error("big-indian CPU is not supported yet.")
#endif

	/* flush to memory or disk depending on open type */
	if (iter->lfh == NULL) {

		/* append new in-memory node with encoded buffer */
		struct invlist_node *node = create_node(key, payload_sz, iter->buf_len);
		memcpy(node->blk, enc_buf, payload_sz);
		append_node(invlist, node, sizeof *node + payload_sz);
		iter->cur = node;
	} else {

		/* write new on-disk block with encoded buffer */
		struct invlist_node *node = create_node(key, payload_sz, iter->buf_len);
		memcpy(node->blk, enc_buf, payload_sz);

		skippy_fwrite(&iter->sfh, &node->sn,
			ondisk_invlist_block_writer, iter->lfh);

		free(node->blk);
		free(node);
	}

	/* reset iterator pointers/buffer */
	iter->buf_idx = 0;
	iter->buf_len = 0;

	return payload_sz;
}

size_t invlist_writer_write(struct invlist_iterator *iter, const void *in)
{
	size_t flush_sz = 0;

	/* flush buffer on inefficient buffer space */
	if (iter->buf_idx + 1 > iter->invlist->buf_max_len)
		flush_sz = invlist_writer_flush(iter);

	/* append to buffer */
	codec_buf_set(iter->buf, iter->buf_idx, (void*)in, iter->c_info);
	iter->buf_idx ++;
	iter->buf_len ++;

	return flush_sz;
}

#include <unistd.h> /* for dup() */
static FILE *invlist_fopen(const char *path, const char *mode, long *len)
{
	char invlist_path[MAX_PATH_LEN];
	sprintf(invlist_path, "%s" ".bin", path);

	FILE *fh = fopen(invlist_path, mode);

	if (len != NULL) {
		int fd = fileno(fh);
		FILE *fh_dup = fdopen(dup(fd), "r");
		fseek(fh_dup, 0, SEEK_END);
		*len = ftell(fh_dup);
		fclose(fh_dup);
	}

	return fh;
}

int invlist_empty(struct invlist* invlist)
{
	if (invlist->type == INVLIST_TYPE_INMEMO) {
		return (invlist->head == NULL);
	} else {
		long len;
		fclose(invlist_fopen(invlist->path, "r", &len));
		return (len == 0);
	}
}

static struct invlist_node *next_blk(struct invlist_node *cur)
{
	struct skippy_node *next = cur->sn.next[0];
	return container_of(next, struct invlist_node, sn);
}

static size_t refill_buffer(struct invlist_iterator *iter)
{
	struct invlist_node *cur = iter->cur;
	size_t out_sz = 0;
	if (cur) {
		codec_buf_struct_info_t *c_info = iter->c_info;
		/* decode and refill buffer */
		out_sz = codec_buf_decode(iter->buf, cur->blk, cur->len, c_info);
		iter->buf_len = cur->len;
	} else {
		iter->buf_len = 0;
	}

	iter->buf_idx = 0;
	return out_sz;
}

static invlist_iter_t base_iterator(struct invlist *invlist)
{
	invlist_iter_t iter = malloc(sizeof *iter);

	iter->buf = codec_buf_alloc(invlist->buf_max_len, invlist->c_info);
	iter->buf_idx = 0;
	iter->buf_len = 0;
	iter->invlist = invlist;
	iter->c_info = invlist->c_info;

	/* undermined type */
	iter->cur = NULL;
	iter->lfh = NULL;

	return iter;
}

static void
iter_persistent_open(invlist_iter_t iter, const char *path, const char *mode)
{
	if (skippy_fopen(&iter->sfh, path, mode, iter->invlist->buf_max_len)) {
		prerr("failed to open on-disk skippy");
		return;
	}

	iter->lfh = invlist_fopen(path, mode, NULL);
}

void invlist_iter_free(struct invlist_iterator *iter)
{
	if (iter->lfh) {
		fclose(iter->lfh);
		skippy_fclose(&iter->sfh);
	}

	codec_buf_free(iter->buf, iter->c_info);
	free(iter);
}

void invlist_writer_free(struct invlist_iterator *iter)
{
	invlist_iter_free(iter);
}

/* an inverted list writer */
invlist_iter_t invlist_writer(struct invlist *invlist)
{
	invlist_iter_t iter = base_iterator(invlist);

	if (invlist->type == INVLIST_TYPE_INMEMO) {
		/* a in-memo invlist */
		iter->cur = invlist->head;
	} else {
		/* a on-disk invlist */
		iter_persistent_open(iter, invlist->path, "a");
	}

	return iter;
}

/* an inverted list reader */
invlist_iter_t invlist_iterator(struct invlist *invlist)
{
	invlist_iter_t iter = base_iterator(invlist);

	if (invlist->type == INVLIST_TYPE_INMEMO) {
		/* a in-memo invlist */
		iter->cur = invlist->head;
		refill_buffer(iter);
	} else {
		/* a on-disk invlist */
		iter_persistent_open(iter, invlist->path, "r");
	}

	return iter;
}

int invlist_iter_next(struct invlist_iterator* iter)
{
	if (iter->buf_idx + 1 < iter->buf_len) {
		iter->buf_idx += 1;
		return 1;

	} else if (iter->cur != NULL) {
		iter->cur = next_blk(iter->cur);
		refill_buffer(iter);
		return (iter->cur != NULL);
	}

	return 0;
}

uint64_t invlist_iter_curkey(struct invlist_iterator* iter)
{
	if (iter->cur == NULL)
		return UINT64_MAX;

	uint32_t key[2], idx = iter->buf_idx;
	codec_buf_struct_info_t *c_info = iter->c_info;

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
	/* assume the order is little-indian */
	CODEC_BUF_GET(key[1], iter->buf, 0, idx, c_info);
	CODEC_BUF_GET(key[0], iter->buf, 1, idx, c_info);
#else
#error("big-indian CPU is not supported yet.")
#endif

	return *(uint64_t*)key;
}

size_t invlist_iter_read(struct invlist_iterator* iter, void *dest)
{
	uint32_t idx = iter->buf_idx;
	size_t out_sz = 0;
	codec_buf_struct_info_t *c_info = iter->c_info;

	for (int j = 0; j < c_info->n_fields; j++) {
		void *field_dest = (char *)dest + c_info->field_info[j].offset;
		CODEC_BUF_CPY(field_dest, iter->buf, j, idx, c_info);
		out_sz += c_info->field_info[j].sz;
	}

	return out_sz;
}

void invlist_print_as_decoded_ints(struct invlist* invlist)
{
	invlist_iter_t iter = invlist_iterator(invlist);
	codec_buf_struct_info_t *c_info = invlist->c_info;

	int cnt = 0;
	do {
		uint64_t key = invlist_iter_curkey(iter);
		uint32_t idx = iter->buf_idx;
		printf("[%14lu]: ", key);

		for (int j = 0; j < c_info->n_fields; j++) {
			void *addr = CODEC_BUF_ADDR(iter->buf, j, idx, c_info);

			switch (c_info->field_info[j].sz) {
			case 1:
				printf("%6u ", *(uint8_t*)addr);
				break;
			case 2:
				printf("%6u ", *(uint16_t*)addr);
				break;
			case 4:
				printf("%6u ", *(uint32_t*)addr);
				break;
			default:
				printf("error! ");
				break;
			}
		}

		cnt ++;
		printf("\n");

	} while (invlist_iter_next(iter));

	invlist_iter_free(iter);
}

//int
//postlist_iter_jump32(struct postlist_iterator* iter, uint32_t target)
//{
//	struct skippy_node *jump_to;
//	uint32_t            id;
//
//	id = *(uint32_t*)(iter->buf + iter->buf_end - iter->item_sz);
//	if (target <= id)
//		goto glide;
//
//	jump_to = skippy_node_lazy_jump(&iter->cur->sn, target);
//
//	/* this is a new block, update pointer and rebuf. */
//	iter->cur = container_of(jump_to, struct postlist_node, sn);
//	postlist_iter_rebuf(iter);
//
//glide:
//	do {
//		id = *(uint32_t*)postlist_iter_cur_item(iter);
//		if (id >= target) return 1;
//
//	} while (postlist_iter_next(iter));
//
//	return 0;
//}
//
//int
//postlist_iter_jump(struct postlist_iterator* iter, uint64_t target)
//{
//	struct skippy_node *jump_to;
//	uint64_t            id;
//
//	id = *(uint64_t*)(iter->buf + iter->buf_end - iter->item_sz);
//	if (target <= id)
//		goto glide;
//
//	jump_to = skippy_node_lazy_jump(&iter->cur->sn, target);
//
//	/* this is a new block, update pointer and rebuf. */
//	iter->cur = container_of(jump_to, struct postlist_node, sn);
//	postlist_iter_rebuf(iter);
//
//glide:
//	do {
//		id = *(uint64_t*)postlist_iter_cur_item(iter);
//		if (id >= target) return 1;
//
//	} while (postlist_iter_next(iter));
//
//	return 0;
//}
