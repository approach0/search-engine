#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include "common/common.h"
#include "postlist/postlist.h"
#include "postlist/math-postlist.h" /* for math_postlist_item */
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "postcalls.h"

/*
 * Math in-memory posting list calls
 */
uint64_t math_memo_postlist_cur(void *iter_)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	return postlist_iter_cur(iter);
}

static int math_memo_postlist_next(void *iter_)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	return postlist_iter_next(iter);
}

static int math_memo_postlist_jump(void *iter_, uint64_t target)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	return postlist_iter_jump(iter, target);
}

static size_t math_memo_postlist_read(void *iter, void *dest, size_t sz)
{
	memcpy(dest, postlist_iter_cur_item(iter), sizeof(struct math_postlist_item));
	return sz;
}

static size_t math_memo_postlist_read_gener(void *iter, void *dest, size_t sz)
{
	memcpy(dest, postlist_iter_cur_item(iter), sizeof(struct math_postlist_gener_item));
	return sz;
}

static void *math_memo_postlist_get_iter(void *po_)
{
	P_CAST(po, struct postlist, po_);
	return postlist_iterator(po);
}

static void math_memo_postlist_del_iter(void *iter_)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	postlist_iter_free(iter);
}

struct postmerger_postlist
math_memo_postlist(void *postlist)
{
	struct postmerger_postlist ret = {
		postlist,
		&math_memo_postlist_cur,
		&math_memo_postlist_next,
		&math_memo_postlist_jump,
		&math_memo_postlist_read,
		&math_memo_postlist_get_iter,
		&math_memo_postlist_del_iter
	};
	return ret;
}

struct postmerger_postlist
math_memo_postlist_gener(void *postlist)
{
	struct postmerger_postlist ret = {
		postlist,
		&math_memo_postlist_cur,
		&math_memo_postlist_next,
		&math_memo_postlist_jump,
		&math_memo_postlist_read_gener,
		&math_memo_postlist_get_iter,
		&math_memo_postlist_del_iter
	};
	return ret;
}

/*
 * Math on-disk posting list calls
 */
static size_t math_disk_postlist_read(void *iter, void *dest, size_t sz)
{
	return math_posting_read(iter, dest);
}

static size_t math_disk_postlist_read_gener(void *iter, void *dest, size_t sz)
{
	return math_posting_read_gener(iter, dest);
}

static void *math_disk_postlist_get_iter(void *path_)
{
	P_CAST(path, char, path_);
	return math_posting_iterator(path);
}

struct postmerger_postlist
math_disk_postlist(void *path)
{
	struct postmerger_postlist ret = {
		path,
		&math_posting_iter_cur,
		&math_posting_iter_next,
		&math_posting_iter_jump,
		&math_disk_postlist_read,
		&math_disk_postlist_get_iter,
		&math_posting_iter_free
	};
	return ret;
}

struct postmerger_postlist
math_disk_postlist_gener(void *path)
{
	struct postmerger_postlist ret = {
		path,
		&math_posting_iter_cur,
		&math_posting_iter_next,
		&math_posting_iter_jump,
		&math_disk_postlist_read_gener,
		&math_disk_postlist_get_iter,
		&math_posting_iter_free
	};
	return ret;
}

/*
 * Term in-memory posting list calls
 */
static uint64_t term_memo_postlist_cur(void *iter_)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	return postlist_iter_cur(iter);
}

static int term_memo_postlist_next(void *iter_)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	return postlist_iter_next(iter);
}

static int term_memo_postlist_jump(void *iter_, uint64_t target)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	return postlist_iter_jump32(iter, target);
}

static size_t term_memo_postlist_read(void *iter, void *dest, size_t sz)
{
	memcpy(dest, postlist_iter_cur_item(iter), sz);
	return sz;
}

static void *term_memo_postlist_get_iter(void *po_)
{
	P_CAST(po, struct postlist, po_);
	return postlist_iterator(po);
}

static void term_memo_postlist_del_iter(void *iter_)
{
	P_CAST(iter, struct postlist_iterator, iter_);
	postlist_iter_free(iter);
}

struct postmerger_postlist
term_memo_postlist(void *postlist)
{
	struct postmerger_postlist ret = {
		postlist,
		&term_memo_postlist_cur,
		&term_memo_postlist_next,
		&term_memo_postlist_jump,
		&term_memo_postlist_read,
		&term_memo_postlist_get_iter,
		&term_memo_postlist_del_iter
	};
	return ret;
}

/*
 * Term on-disk posting list calls
 */

static uint64_t term_disk_postlist_cur(void *po)
{
	return (int)term_posting_cur(po);
}

static int term_disk_postlist_next(void *po)
{
	return (int)term_posting_next(po);
}

static int term_disk_postlist_jump(void *po, uint64_t target)
{
	return (int)term_posting_jump(po, target);
}

static size_t term_disk_postlist_read(void *po, void *dest, size_t sz)
{
	return term_posting_read(po, dest);
}

static void* term_disk_postlist_get_iter(void *po)
{
	if (term_posting_start(po)) return po;
	else return NULL;
}

static void term_disk_postlist_del_iter(void *po)
{
	return;
}

void term_postlist_free(struct postmerger_postlist po)
{
	if (po.get_iter == &term_disk_postlist_get_iter)
		term_posting_finish(po.po);
}

struct postmerger_postlist
term_disk_postlist(void *po)
{
	struct postmerger_postlist ret = {
		po,
		&term_disk_postlist_cur,
		&term_disk_postlist_next,
		&term_disk_postlist_jump,
		&term_disk_postlist_read,
		&term_disk_postlist_get_iter,
		&term_disk_postlist_del_iter
	};
	return ret;
}

/*
 * Empty posting list calls
 */

static uint64_t empty_postlist_cur(void *iter)
{
	return UINT64_MAX;
}

static int empty_postlist_next(void *iter)
{
	return 0;
}

static int empty_postlist_jump(void *iter, uint64_t target)
{
	return 0;
}

static size_t empty_postlist_read(void *iter, void *dest, size_t sz)
{
	assert(0);
	return 0;
}

static void *empty_postlist_init(void *iter)
{
	return NULL;
}

static void empty_postlist_uninit(void *iter)
{
	return;
}

struct postmerger_postlist
empty_postlist()
{
	struct postmerger_postlist ret = {
		NULL,
		&empty_postlist_cur,
		&empty_postlist_next,
		&empty_postlist_jump,
		&empty_postlist_read,
		&empty_postlist_init,
		&empty_postlist_uninit
	};
	return ret;
}
