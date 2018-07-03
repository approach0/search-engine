#pragma once
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include "config.h"

#define MAX_POST_ITEM_ID   ULLONG_MAX

struct postmerge;

typedef void*          post_item_t;

typedef void           (*post_finish_callbk)(void *);
typedef bool           (*post_start_callbk)(void *);
typedef bool           (*post_jump_callbk)(void *, uint64_t);
typedef bool           (*post_next_callbk)(void *);
typedef post_item_t    (*post_cur_callbk)(void *);
typedef uint64_t       (*post_cur_id_callbk)(post_item_t);
typedef int            (*post_merge_callbk)(uint64_t, struct postmerge*, void*);

enum postmerge_op {
	POSTMERGE_OP_UNDEF,
	POSTMERGE_OP_AND,
	POSTMERGE_OP_OR,
};

struct postmerge {
	void               *postings[MAX_MERGE_POSTINGS];
	void               *posting_args[MAX_MERGE_POSTINGS];
	uint64_t            curIDs[MAX_MERGE_POSTINGS];
	uint32_t            n_postings;
	int64_t             n_rd_items;

	post_start_callbk   start[MAX_MERGE_POSTINGS];
	post_next_callbk    next[MAX_MERGE_POSTINGS];
	post_jump_callbk    jump[MAX_MERGE_POSTINGS];
	post_cur_callbk     cur[MAX_MERGE_POSTINGS];
	post_cur_id_callbk  cur_id[MAX_MERGE_POSTINGS];
	post_finish_callbk  finish[MAX_MERGE_POSTINGS];

	post_merge_callbk   post_on_merge;
};

struct postmerge_callbks {
	/* The posting start function will place the current pointer
	 * at the beginning of posting list. */
	post_start_callbk start;

	/*
	 * The posting next function, will go one item further, and
	 * return whether or not we are still in the buffer.
	 */
	post_next_callbk next;

	/*
	 * The posting jump function, is defined something like this:
	 * do {
	 *     if (item[i].ID >= target_ID)
	 *         return true;
	 * } while (posting_next());
	 *
	 * In another words, jump to the item whose ID >= a given ID.
	 */
	post_jump_callbk jump;

	/*
	 * Note: after iterator goes out of scope, postmerge.c module
	 * guarantees next() and jump() will not be
	 * called again.
	 */

	/* get current posting item */
	post_cur_callbk cur;

	/* get docID of a posting item */
	post_cur_id_callbk cur_id;

	/*
	 * Note: after iterator goes out of scope, postmerge.c module
	 * guarantees cur() and cur_id() will not be
	 * called.
	 * Thus, never try to use things like:
	 *   while ((pi = term_posting_current(posting)) != NULL)
	 * because cur() callback is not supposed to implement
	 * any NULL indicator as a flag of "end of posting list".
	 */

	/* posting list release function */
	post_finish_callbk finish;
};

#define NULL_POSTMERGE_CALLS ((struct postmerge_callbks) {0})
#define POSTMERGE_CUR(_pm, _i) ((_pm)->cur[_i]((_pm)->postings[_i]))

void postmerge_posts_clear(struct postmerge*);

/*
 * Even if a posting list is empty, you need to call postmerge_posts_add()
 * to add it for merge process, because AND merge may need NULL pointer to
 * indicate posting list is empty such that no results is going to be
 * returned. */
void postmerge_posts_add(struct postmerge*, void*,
                         struct postmerge_callbks, void*);

/* posting list IDs must be incremental and *unique* for each item */
bool posting_merge(struct postmerge*, enum postmerge_op,
                   post_merge_callbk, void*);
