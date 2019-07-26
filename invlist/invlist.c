#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//#include "config.h"
#include <assert.h>

#include "common/common.h"
//#include "postlist.h"

///* buffer setup/free macro */
#define SETUP_BUFFER(_po) \
	if (_po->buf == NULL) { \
		_po->buf = malloc(_po->buf_sz); \
		_po->tot_sz += _po->buf_sz; \
	} do {} while (0)

#define FREE_BUFFER(_po) \
	if (_po->buf) { \
		free(_po->buf); \
		_po->buf = NULL; \
		_po->tot_sz -= _po->buf_sz; \
	} do {} while (0)

//void postlist_print_info(struct postlist *po)
//{
//	printf("==== memory posting list info ====\n");
//	printf("%u blocks (%.2f KB).\n", po->n_blk,
//	       (float)po->tot_sz / 1024.f);
//	printf("item size: %u B\n", po->item_sz);
//	printf("buffer size: %u B ", po->buf_sz);
//	printf("(%u items / buffer)\n", po->buf_sz / po->item_sz);
//
//	skippy_print(&po->skippy, 0);
//}
//
//struct postlist *
//postlist_create(uint32_t skippy_spans, uint32_t buf_sz, uint32_t item_sz,
//                void *buf_arg, struct postlist_callbks calls)
//{
//	struct postlist *ret;
//	ret = malloc(sizeof(struct postlist));
//
//	/* assign initial values */
//	ret->head = ret->tail = NULL;
//	ret->tot_sz = sizeof(struct postlist);
//	ret->n_blk  = 0;
//	skippy_init(&ret->skippy, skippy_spans);
//
//	ret->buf = NULL;
//	ret->buf_sz = buf_sz;
//	ret->buf_end = 0;
//	ret->buf_arg = buf_arg;
//
//	ret->on_flush = calls.on_flush;
//	ret->on_rebuf = calls.on_rebuf;
//	ret->on_free  = calls.on_free;
//
//	/* leave iterator-related initializations to postlist_start() */
//	ret->item_sz = item_sz;
//
//	return ret;
//}
//
//static struct postlist_node *create_node(uint64_t key, size_t size)
//{
//	struct postlist_node *ret;
//	ret = malloc(sizeof(struct postlist_node));
//
//	/* assign initial values */
//	skippy_node_init(&ret->sn, key);
//	ret->blk = malloc(size);
//	ret->blk_sz = size;
//
//	return ret;
//}
//
//static void
//append_node(struct postlist *po, struct postlist_node *node)
//{
//	if (po->head == NULL)
//		po->head = node;
//
//	po->tail = node;
//	po->tot_sz += node->blk_sz;
//	po->n_blk ++;
//
//#ifdef DEBUG_POSTLIST
//	//printf("appending skippy node...\n");
//#endif
//	skippy_append(&po->skippy, &node->sn);
//}
//
//static uint32_t postlist_flush(struct postlist *po)
//{
//	uint64_t flush_key;
//	uint32_t flush_sz;
//	struct postlist_node *node;
//
//	/* invoke flush callback and get flush size */
//	flush_key = po->on_flush(po->buf, &po->buf_end, po->buf_arg);
//	flush_sz = po->buf_end;
//
//	/* append new node with copy of current buffer */
//	node = create_node(flush_key, flush_sz);
//	append_node(po, node);
//
//	/* flush to new tail node */
//	memcpy(node->blk, po->buf, flush_sz);
//
//	/* reset buffer */
//	po->buf_end = 0;
//
//	return flush_sz;
//}
//
//size_t
//postlist_write(struct postlist *po, const void *in, size_t size)
//{
//	size_t flush_sz = 0;
//
//	/* setup buffer for writting */
//	SETUP_BUFFER(po);
//
//	/* flush buffer under inefficient buffer space */
//	if (po->buf_end + size > po->buf_sz)
//		flush_sz = postlist_flush(po);
//
//	/* write into buffer */
//	assert(po->buf_end + size <= po->buf_sz);
//	memcpy(po->buf + po->buf_end, in, size);
//	po->buf_end += size;
//
//#ifdef DEBUG_POSTLIST
//	printf("buffer after writting: [%u/%u]\n",
//	       po->buf_end, po->buf_sz);
//#endif
//
//	return flush_sz;
//}
//
//size_t postlist_write_complete(struct postlist *po)
//{
//	size_t flush_sz = 0;
//
//	if (po->buf)
//		flush_sz = postlist_flush(po);
//
//	FREE_BUFFER(po);
//	return flush_sz;
//}
//
//void postlist_free(struct postlist *po)
//{
//	po->on_free(po->buf_arg);
//	skippy_free(&po->skippy, struct postlist_node, sn,
//	            free(p->blk); free(p));
//	free(po->buf);
//	free(po);
//}
//
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
//	iter->buf = malloc(po->buf_sz);
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
