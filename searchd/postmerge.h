#pragma once
#include <stdint.h>
#include <limits.h>
#include "config.h"

#define MAX_POST_ITEM_ID   ULLONG_MAX

struct postmerge_arg;

typedef void*          post_item_t;

typedef void           (*post_finish_callbk)(void *);
typedef bool           (*post_start_callbk)(void *);
typedef bool           (*post_jump_callbk)(void *, uint64_t);
typedef bool           (*post_next_callbk)(void *);
typedef post_item_t    (*post_now_callbk)(void *);
typedef uint64_t       (*post_now_id_callbk)(post_item_t);
typedef void           (*post_merge_callbk)(uint64_t, struct postmerge_arg*, void*);

enum postmerge_op {
	POSTMERGE_OP_UNDEF,
	POSTMERGE_OP_AND,
	POSTMERGE_OP_OR,
};

struct postmerge_arg {
	void               *postings[MAX_MERGE_POSTINGS];
	void               *posting_args[MAX_MERGE_POSTINGS];
	uint64_t            curIDs[MAX_MERGE_POSTINGS];
	void               *cur_pos_item[MAX_MERGE_POSTINGS];
	uint32_t            n_postings;
	enum postmerge_op   op;

	/* The posting start function will place the current pointer
	 * at the beginning of posting list. */
	post_start_callbk   post_start_fun;

	/*
	 * The posting next function, will go one item further, and
	 * return whether or not we are still in the buffer.
	 */
	post_next_callbk    post_next_fun;

	/*
	 * The posting jump function, is defined something like this:
	 * do {
	 *     if (item[i].ID >= target_ID)
	 *         return true;
	 * } while (posting_next());
	 *
	 * In another words, jump to the item whose ID >= a given ID.
	 */
	post_jump_callbk    post_jump_fun;

	/*
	 * Note: after iterator goes out of scope, postmerge.c module
	 * guarantees post_next_fun() and post_jump_fun() will not be
	 * called again.
	 */

	/* get current posting item */
	post_now_callbk     post_now_fun;

	/* get docID of a posting item */
	post_now_id_callbk  post_now_id_fun;

	/*
	 * Note: after iterator goes out of scope, postmerge.c module
	 * guarantees post_now_fun() and post_now_id_fun() will not be
	 * called.
	 * Thus, never try to use things like:
	 *   while ((pi = term_posting_current(posting)) != NULL)
	 * because post_now_fun() callback is not supposed to implement
	 * any NULL indicator as a flag of "end of posting list".
	 */

	/* posting list release function */
	post_finish_callbk  post_finish_fun;

	/* posting list merge callback */
	post_merge_callbk   post_on_merge;
};

void postmerge_arg_init(struct postmerge_arg*);

/*
 * Even if a posting list is empty, you need to call postmerge_arg_add_post()
 * to add it for merge process, because AND merge may need NULL pointer to
 * indicate posting list is empty such that no results is going to be returned. */
void postmerge_arg_add_post(struct postmerge_arg*, void*, void*);

/* posting list IDs must be incremental and *unique* for each item */
bool posting_merge(struct postmerge_arg*, void*);
