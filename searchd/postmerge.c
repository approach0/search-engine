#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "postmerge.h"

void postmerge_arg_init(struct postmerge_arg *arg)
{
	memset(arg, 0, sizeof(struct postmerge_arg));
}

void postmerge_arg_add_post(struct postmerge_arg *pm_arg, void *post, void *arg)
{
	pm_arg->postings[pm_arg->n_postings] = post;
	pm_arg->posting_args[pm_arg->n_postings] = arg;
	pm_arg->curIDs[pm_arg->n_postings] = 0;
	pm_arg->n_postings ++;
}

static bool
update_min_idx(struct postmerge_arg *arg, uint32_t *cur_min_idx)
{
	uint32_t i;
	uint64_t cur_min = MAX_POST_ITEM_ID;

	for (i = 0; i < arg->n_postings; i++) {
		if (arg->curIDs[i] < cur_min) {
			cur_min = arg->curIDs[i];
			*cur_min_idx = i;
		}
	}

	return (cur_min != MAX_POST_ITEM_ID);
}

static bool
update_minmax_idx(struct postmerge_arg *arg,
                  uint32_t *cur_min_idx, uint32_t *cur_max_idx)
{
	uint32_t i;
	uint64_t cur_max = 0;
	uint64_t cur_min = MAX_POST_ITEM_ID;

	for (i = 0; i < arg->n_postings; i++) {
		if (arg->curIDs[i] < cur_min) {
			cur_min = arg->curIDs[i];
			*cur_min_idx = i;
		}

		if (arg->curIDs[i] > cur_max) {
			cur_max = arg->curIDs[i];
			*cur_max_idx = i;
		}
	}

	return (cur_min != MAX_POST_ITEM_ID);
}

static bool
next_id_OR(struct postmerge_arg *arg, uint32_t *cur_min_idx)
{
	uint32_t i;
	uint64_t cur_min = arg->curIDs[*cur_min_idx];

	for (i = 0; i < arg->n_postings; i++) {
		if (arg->curIDs[i] <= cur_min) {
			arg->post_next_fun(arg->postings[i]);
			arg->cur_pos_item[i] = arg->post_now_fun(arg->postings[i]);
			arg->curIDs[i] = arg->post_now_id_fun(arg->cur_pos_item[i]);
		}
	}

	return update_min_idx(arg, cur_min_idx);
}

static bool
next_id_AND(struct postmerge_arg *arg,
        uint32_t *cur_min_idx, uint32_t *cur_max_idx)
{
	uint32_t i;
	uint64_t cur_min = arg->curIDs[*cur_min_idx];
	uint64_t cur_max = arg->curIDs[*cur_max_idx];

	for (i = 0; i < arg->n_postings; i++) {
		if (arg->curIDs[i] <= cur_min) {
			if (arg->curIDs[i] == cur_max)
				arg->post_next_fun(arg->postings[i]);
			else
				if (!arg->post_jump_fun(arg->postings[i], cur_max))
					break;

			arg->cur_pos_item[i] = arg->post_now_fun(arg->postings[i]);
			arg->curIDs[i] = arg->post_now_id_fun(arg->cur_pos_item[i]);
		}
	}

	if (i == arg->n_postings)
		return update_minmax_idx(arg, cur_min_idx, cur_max_idx);
	else
		return 0;
}

static void
posting_merge_OR(struct postmerge_arg *arg, void *extra_args)
{
	uint32_t cur_min_idx = 0;
	uint64_t cur_min;

	update_min_idx(arg, &cur_min_idx);

	do {
		cur_min = arg->curIDs[cur_min_idx];
		arg->post_on_merge(cur_min, arg, extra_args);

	} while (next_id_OR(arg, &cur_min_idx));
}

static void
posting_merge_AND(struct postmerge_arg *arg, void *extra_args)
{
	uint32_t i, cur_min_idx = 0, cur_max_idx = 0;
	uint64_t cur_min;

	update_minmax_idx(arg, &cur_min_idx, &cur_max_idx);

	do {
		cur_min = arg->curIDs[cur_min_idx];
		for (i = 0; i < arg->n_postings; i++)
			if (arg->curIDs[i] != cur_min)
				break;

		if (i == arg->n_postings)
			arg->post_on_merge(cur_min, arg, extra_args);

	} while (next_id_AND(arg, &cur_min_idx, &cur_max_idx));
}

bool posting_merge(struct postmerge_arg *arg, void *extra_args)
{
	uint32_t i;

	if (arg->n_postings == 0)
		return 1;

	/* initialize posting iterator */
	for (i = 0; i < arg->n_postings; i++) {
		arg->post_start_fun(arg->postings[i]);
		arg->cur_pos_item[i] = arg->post_now_fun(arg->postings[i]);
		arg->curIDs[i] = arg->post_now_id_fun(arg->cur_pos_item[i]);
	}

	if (arg->op == POSTMERGE_OP_AND)
		posting_merge_AND(arg, extra_args);
	else if (arg->op == POSTMERGE_OP_OR)
		posting_merge_OR(arg, extra_args);
	else
		return 0;

	/* un-initialize posting iterator */
	for (i = 0; i < arg->n_postings; i++)
		arg->post_finish_fun(arg->postings[i]);

	return 1;
}
