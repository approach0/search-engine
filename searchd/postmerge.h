#pragma once
#include <stdint.h>
#include <limits.h>
#include "config.h"

#define MAX_POST_ITEM_ID   ULLONG_MAX

struct postmerge_arg;

typedef void*          post_item_t;

typedef void           (*post_do_callbk)(void *);
typedef bool           (*post_jump_callbk)(void *, uint64_t);
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
	uint64_t            curIDs[MAX_MERGE_POSTINGS];
	void               *cur_pos_item[MAX_MERGE_POSTINGS];
	uint32_t            n_postings;
	void               *extra_args;
	enum postmerge_op   op;

	/* posting list current item move */
	post_do_callbk      post_start_fun;
	post_jump_callbk    post_jump_fun; /* jump to the item whose ID >= a given ID */
	post_do_callbk      post_next_fun; /* go to the next item */
	post_do_callbk      post_finish_fun;

	/* posting list 'get' functions */
	post_now_callbk     post_now_fun;  /* get current posting item */
	post_now_id_callbk  post_now_id_fun; /* get docID of a posting item */

	/* posting list merge callback */
	post_merge_callbk   post_on_merge;
};

void postmerge_arg_init(struct postmerge_arg*);

/*
 * Even if a posting list is empty, you need to call postmerge_arg_add_post()
 * to add it for merge process, because AND merge may need NULL pointer to
 * indicate posting list is empty such that no results is going to be returned. */
void postmerge_arg_add_post(struct postmerge_arg*, void*);

/* posting list IDs must be incremental and *unique* for each item */
bool posting_merge(struct postmerge_arg*, void*);
