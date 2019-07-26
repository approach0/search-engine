#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#include "config.h"
#include <assert.h>

#include "common/common.h"
#include "invlist.h"

//void postlist_print_info(struct postlist *po)
//{
//	printf("==== memory posting list info ====\n");
//	printf("%u blocks (%.2f KB).\n", po->n_blk,
//	       (float)po->tot_sz / 1024.f);
//	printf("item size: %u B\n", po->item_sz);
//	printf("buffer size: %u B ", po->buf_max_sz);
//	printf("(%u items / buffer)\n", po->buf_max_sz / po->item_sz);
//
//	skippy_print(&po->skippy, 0);
//}

struct invlist *invlist_create(uint32_t n, codec_buf_struct_info_t* c_info)
{
	struct invlist *ret = malloc(sizeof *ret);

	/* skippy book keeping */
	ret->head = NULL;
	ret->tot_sz = sizeof *ret;
	ret->n_blk = 0;
	skippy_init(&ret->skippy, n);
	ret->buf_max_len = n;
	ret->buf_max_sz = 0;
	ret->c_info = c_info;

	for (int i = 0; i < c_info->n_fields; i++) {
		struct field_info info = c_info->field_info[i];
		ret->buf_max_sz += n * info.sz;
	}

	return ret;
}

static struct invlist_node *create_node(uint64_t key, size_t size)
{
	struct invlist_node *ret = malloc(sizeof *ret);

	/* assign initial values */
	skippy_node_init(&ret->sn, key);
	ret->blk = malloc(size);
	ret->blk_sz = size;

	return ret;
}

static void append_node(struct invlist *invlist, struct invlist_node *node)
{
	if (invlist->head == NULL)
		invlist->head = node;
	invlist->tot_sz += node->blk_sz;
	invlist->n_blk ++;
	skippy_append(&invlist->skippy, &node->sn);
}

size_t invlist_iter_flush(struct invlist_iterator *iter)
{
	struct invlist *invlist = iter->invlist;
	size_t sz;

	/* encode current buffer integers */
	char enc_buf[invlist->buf_max_sz];
	sz = codec_buf_encode(enc_buf, iter->buf, iter->buf_len, iter->c_info);

	/* append new node with copy of current buffer */
	uint64_t key = *(uint64_t *)(iter->buf[0]);
	struct invlist_node *node = create_node(key, sz);
	append_node(invlist, node);
	memcpy(node->blk, enc_buf, sz);

	/* reset iterator pointers/buffer */
	iter->cur = node;
	iter->buf_idx = 0;
	iter->buf_len = 0;

	return sz;
}

size_t invlist_iter_write(struct invlist_iterator *iter, const void *in)
{
	struct invlist *invlist = iter->invlist;

	/* setup buffer for writting */
	if (iter->buf == NULL)
		iter->buf = codec_buf_alloc(invlist->buf_max_len, iter->c_info);

	/* flush buffer on inefficient buffer space */
	if (iter->buf_idx + 1 > invlist->buf_max_len)
		invlist_iter_flush(iter);

	/* append to buffer */
	codec_buf_set(iter->buf, iter->buf_idx, (void*)in, iter->c_info);
	iter->buf_idx ++;
	iter->buf_len ++;

	return 0;
}

void invlist_free(struct invlist *invlist)
{
	skippy_free(&invlist->skippy, struct invlist_node, sn, free(p->blk); free(p));
	free(invlist);
}

void invlist_iter_free(struct invlist_iterator *iter)
{
	if (iter->buf)
		codec_buf_free(iter->buf, iter->c_info);
}

//static void forward_cur(struct postlist_node **cur)
//{
//	struct skippy_node *next = (*cur)->sn.next[0];
//
//	/* forward one step */
//	*cur = container_of(next, struct postlist_node, sn);
//}
//
//static void
//postlist_iter_rebuf(struct postlist_iterator *iter)
//{
//	struct postlist_node *cur = iter->cur;
//	if (cur) {
//		/* refill buffer */
//		memcpy(iter->buf, cur->blk, cur->blk_sz);
//		iter->buf_end = cur->blk_sz;
//
//		/* invoke decompression */
//		iter->on_rebuf(iter->buf, &iter->buf_end, iter->buf_arg);
//	} else {
//		/* reset buffer variables anyway */
//		iter->buf_end = 0;
//	}
//
//	iter->buf_idx = 0;
//}
//
///*
// * Postlist iterators (reentrant version)
// */
//
//postlist_iter_t postlist_iterator(struct postlist *po)
//{
//	struct postlist_iterator *iter;
//	iter = malloc(sizeof(struct postlist_iterator));
//	iter->cur = po->head;
//
//	iter->on_rebuf = po->on_rebuf;
//	iter->buf_arg = po->buf_arg;
//
//	iter->buf = malloc(po->buf_max_sz);
//	iter->buf_idx = 0;
//	iter->buf_end = 0;
//	iter->item_sz = po->item_sz;
//
//	/* initial buffer filling */
//	postlist_iter_rebuf(iter);
//
//	return iter;
//}
//
//void postlist_iter_free(struct postlist_iterator* iter)
//{
//	free(iter->buf);
//	free(iter);
//}
//
//int postlist_empty(struct postlist* po)
//{
//	return (po->head == NULL);
//}
//
//int postlist_iter_next(struct postlist_iterator* iter)
//{
//	if (iter->buf_idx + iter->item_sz < iter->buf_end) {
//		iter->buf_idx += iter->item_sz;
//		return 1;
//
//	} else if (iter->buf_end != 0) {
//		/* next block */
//		forward_cur(&iter->cur);
//		/* reset buffer */
//		postlist_iter_rebuf(iter);
//		return (iter->buf_end != 0);
//	}
//
//	return 0;
//}
//
//void* postlist_iter_cur_item(struct postlist_iterator* iter)
//{
//	return iter->buf + iter->buf_idx;
//}
//
//uint64_t postlist_iter_cur(struct postlist_iterator* iter)
//{
//	if (iter->buf_end == 0)
//		return UINT64_MAX;
//	else
//		return *(uint64_t*)postlist_iter_cur_item(iter);
//}
//
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
//
//#include "postlist-codec/postlist-codec.h"
//
//void print_postlist(struct postlist *po)
//{
//	PTR_CAST(codec, struct postlist_codec, po->buf_arg);
//
//	postlist_iter_t iter = postlist_iterator(po);
//	do {
//		void *pi = postlist_iter_cur_item(iter);
//		postlist_print(pi, 1, codec->fields);
//	} while (postlist_iter_next(iter));
//	postlist_iter_free(iter);
//}
