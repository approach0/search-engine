#include "postlist/postlist.h"
#include "term-index/term-index.h"
#include "mergecalls.h"

static uint64_t wrap_mem_term_postlist_cur_id(void *item)
{
	/* return docID */
	return (uint64_t)(*(uint32_t*)item);
}

struct postmerge_callbks mergecalls_mem_term_postlist()
{
	struct postmerge_callbks ret;
	ret.start  = &postlist_start;
	ret.finish = &postlist_finish;
	ret.jump   = &postlist_jump;
	ret.next   = &postlist_next;
	ret.now    = &postlist_cur_item;
	ret.now_id = &wrap_mem_term_postlist_cur_id;

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

static uint64_t wrap_disk_term_postlist_cur_id(void *item)
{
	return (uint64_t)(*(uint32_t*)item);
}

struct postmerge_callbks mergecalls_disk_term_postlist()
{
	struct postmerge_callbks ret;
	ret.start  = &term_posting_start;
	ret.finish = &term_posting_finish;
	ret.jump   = &wrap_disk_term_postlist_jump;
	ret.next   = &term_posting_next;
	ret.now    = &wrap_disk_term_postlist_cur_item;
	ret.now_id = &wrap_disk_term_postlist_cur_id;

	return ret;
}
