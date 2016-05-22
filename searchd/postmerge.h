#pragma once
#include <stdint.h>
#include <limits.h>
#include "config.h"

#define MAX_POST_ITEM_ID   ULLONG_MAX

struct postmerge;

typedef void*          post_item_t;

typedef void           (*post_finish_callbk)(void *);
typedef bool           (*post_start_callbk)(void *);
typedef bool           (*post_jump_callbk)(void *, uint64_t);
typedef bool           (*post_next_callbk)(void *);
typedef post_item_t    (*post_now_callbk)(void *);
typedef uint64_t       (*post_now_id_callbk)(post_item_t);
typedef void           (*post_merge_callbk)(uint64_t, struct postmerge*, void*);

enum postmerge_op {
	POSTMERGE_OP_UNDEF,
	POSTMERGE_OP_AND,
	POSTMERGE_OP_OR,
};

struct postmerge {
	void               *postings[MAX_MERGE_POSTINGS];
	void               *posting_args[MAX_MERGE_POSTINGS];
	uint64_t            curIDs[MAX_MERGE_POSTINGS];
	void               *cur_pos_item[MAX_MERGE_POSTINGS];
	uint32_t            n_postings;

	post_start_callbk   start[MAX_MERGE_POSTINGS];
	post_next_callbk    next[MAX_MERGE_POSTINGS];
	post_jump_callbk    jump[MAX_MERGE_POSTINGS];
	post_now_callbk     now[MAX_MERGE_POSTINGS];
	post_now_id_callbk  now_id[MAX_MERGE_POSTINGS];
	post_finish_callbk  finish[MAX_MERGE_POSTINGS];
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
	post_now_callbk now;

	/* get docID of a posting item */
	post_now_id_callbk now_id;

	/*
	 * Note: after iterator goes out of scope, postmerge.c module
	 * guarantees now() and now_id() will not be
	 * called.
	 * Thus, never try to use things like:
	 *   while ((pi = term_posting_current(posting)) != NULL)
	 * because now() callback is not supposed to implement
	 * any NULL indicator as a flag of "end of posting list".
	 */

	/* posting list release function */
	post_finish_callbk finish;
};


void postmerge_posts_clear(struct postmerge*);

/*
 * Even if a posting list is empty, you need to call postmerge_posts_add()
 * to add it for merge process, because AND merge may need NULL pointer to
 * indicate posting list is empty such that no results is going to be
 * returned. */
void postmerge_posts_add(struct postmerge*, void*,
                         struct postmerge_callbks*, void*);

/* posting list IDs must be incremental and *unique* for each item */
bool posting_merge(struct postmerge*, enum postmerge_op,
                   post_merge_callbk, void*);
