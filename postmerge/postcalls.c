#include <stddef.h>
#include <stdlib.h>
#include "common/common.h"
#include "postlist/postlist.h"
#include "postlist/math-postlist.h" /* for math_postlist_item */
#include "postlist-cache/math-postlist-cache.h" /* for disk_item_v2_to_mem_item */
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "postcalls.h"

static uint64_t math_memo_postlist_cur(void *po)
{
	if (postlist_terminates(po))
		return UINT64_MAX;
	else
		return *(uint64_t*)postlist_cur_item(po);
}

static int math_memo_postlist_next(void *po)
{
	return (int)postlist_next(po);
}

static int math_memo_postlist_jump(void *po, uint64_t target)
{
	return (int)postlist_jump(po, target);
}

static size_t math_memo_postlist_read(void *po, void *dest, size_t sz)
{
	memcpy(dest, postlist_cur_item(po), sz);
	return sz;
}

static int math_memo_postlist_init(void *po)
{
	return (int)postlist_start(po);
}

static void math_memo_postlist_uninit(void *po)
{
	postlist_finish(po);
}

struct postmerger_postlist
math_memo_postlist(void *po)
{
	struct postmerger_postlist ret = {
		po,
		&math_memo_postlist_cur,
		&math_memo_postlist_next,
		&math_memo_postlist_jump,
		&math_memo_postlist_read,
		&math_memo_postlist_init,
		&math_memo_postlist_uninit
	};
	return ret;
}

static uint64_t math_disk_postlist_cur(void *po)
{
	return math_posting_cur_id_v2_2(po);
}

static int math_disk_postlist_next(void *po)
{
	return (int)math_posting_next(po);
}

static int math_disk_postlist_jump(void *po, uint64_t target)
{
	return (int)math_posting_jump(po, target);
}

static size_t math_disk_postlist_read(void *po, void *dest, size_t sz)
{
	PTR_CAST(disk_item, struct math_posting_compound_item_v2,
	         math_posting_cur_item_v2(po));
	disk_item_v2_to_mem_item(dest, disk_item);
	return sizeof(struct math_postlist_item);
}

static int math_disk_postlist_init(void *po)
{
	return (int)math_posting_start(po);
}

static void math_disk_postlist_uninit(void *po)
{
	math_posting_finish(po);
	math_posting_free_reader(po);
}

struct postmerger_postlist
math_disk_postlist(void *po)
{
	struct postmerger_postlist ret = {
		po,
		&math_disk_postlist_cur,
		&math_disk_postlist_next,
		&math_disk_postlist_jump,
		&math_disk_postlist_read,
		&math_disk_postlist_init,
		&math_disk_postlist_uninit
	};
	return ret;
}
