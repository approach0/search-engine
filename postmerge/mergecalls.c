#include "common/common.h"
#include "postlist/postlist.h"
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "mergecalls.h"

/*
 * Text posting list merge calls.
 */

static uint64_t wrap_mem_term_postlist_cur_id(void *po)
{
	/* return docID */
	return (uint64_t)(*(uint32_t*)postlist_cur_item(po));
}

struct postmerge_callbks mergecalls_mem_term_postlist()
{
	struct postmerge_callbks ret;
	ret.start  = &postlist_start;
	ret.finish = &postlist_finish;
	ret.jump   = &postlist_jump;
	ret.next   = &postlist_next;
	ret.cur    = &postlist_cur_item;
	ret.cur_id = &wrap_mem_term_postlist_cur_id;

	return ret;
}

static bool wrap_disk_term_postlist_jump(void *po, uint64_t to_id)
{
	/* because uint64_t value can be greater than doc_id_t,
	 * we need a wrapper function to safe-guard from
	 * calling term_posting_jump with illegal argument. */
	if (to_id >= UINT_MAX)
		return 0;
	else
		return term_posting_jump(po, (doc_id_t)to_id);
}

static void *wrap_disk_term_postlist_cur_item(void *po)
{
	return (void*)term_posting_cur_item_with_pos(po);
}

static uint64_t wrap_disk_term_postlist_cur_id(void *po)
{
	return (uint64_t)(*(uint32_t*)term_posting_cur_item_with_pos(po));
}

struct postmerge_callbks mergecalls_disk_term_postlist()
{
	struct postmerge_callbks ret;
	ret.start  = &term_posting_start;
	ret.finish = &term_posting_finish;
	ret.jump   = &wrap_disk_term_postlist_jump;
	ret.next   = &term_posting_next;
	ret.cur    = &wrap_disk_term_postlist_cur_item;
	ret.cur_id = &wrap_disk_term_postlist_cur_id;

	return ret;
}

/*
 * Math posting list merge calls.
 */

static uint64_t wrap_mem_math_postlist_cur_id(void *po)
{
	return *(uint64_t*)postlist_cur_item(po);
}

struct postmerge_callbks mergecalls_mem_math_postlist()
{
	struct postmerge_callbks ret;
	ret.start  = &postlist_start;
	ret.finish = &postlist_finish;
	ret.jump   = &postlist_jump;
	ret.next   = &postlist_next;
	ret.cur    = &postlist_cur_item;
	ret.cur_id = &wrap_mem_math_postlist_cur_id;

	return ret;
}

#include "postlist/math-postlist.h"
#include "postlist-cache/math-postlist-cache.h"
#include "postmerge/config.h"
char *trans_symbol(int symbol_id);

struct postmerge_callbks mergecalls_disk_math_postlist_v1()
{
	struct postmerge_callbks ret;
	ret.start  = &math_posting_start;
	ret.finish = &math_posting_finish;
	ret.jump   = &math_posting_jump;
	ret.next   = &math_posting_next;
	ret.cur    = &math_posting_cur_item_v1;
	ret.cur_id = &math_posting_cur_id_v1;

	return ret;
}

struct postmerge_callbks mergecalls_disk_math_postlist_v2()
{
	struct postmerge_callbks ret;
	ret.start  = &math_posting_start;
	ret.finish = &math_posting_finish;
	ret.jump   = &math_posting_jump;
	ret.next   = &math_posting_next;
	ret.cur    = NULL; /* deprecated */
	ret.cur_id = &math_posting_cur_id_v2;

	return ret;
}
