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
			/* now, move head posting list iterator[i] */
			if (arg->post_next_fun(arg->postings[i])) {
				/* update current pointer and ID for posting[i] */
				arg->cur_pos_item[i] = arg->post_now_fun(arg->postings[i]);
				arg->curIDs[i] = arg->post_now_id_fun(arg->cur_pos_item[i]);
			} else {
				/* for OR, we cannot just simply leave curIDs[i] unchanged,
				 * because in posting_merge_OR() will continue to evaluate
				 * min value.
				 */
				arg->cur_pos_item[i] = NULL;
				arg->curIDs[i] = MAX_POST_ITEM_ID;
			}
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
			/* now, move head posting list iterator[i] */
			if (arg->curIDs[i] == cur_max) {
				/* in this case we do not jump() because if all
				 * curIDs[i] are equal, jump() will stuck here
				 * forever. */
				if (!arg->post_next_fun(arg->postings[i]))
					/* iterator goes out of posting list scope
					 * i.e. curIDs[i] now is infinity, no need to
					 * go to next posting_merge_AND() iteration.
					 */
					return 0;
			} else {
				/* do jump for the sake of efficiency (although
				 * depend on jump() callback implementation) */
				if (!arg->post_jump_fun(arg->postings[i], cur_max))
					/* similarly */
					return 0;
			}

			/* update current pointer and ID for posting[i] */
			arg->cur_pos_item[i] = arg->post_now_fun(arg->postings[i]);
			arg->curIDs[i] = arg->post_now_id_fun(arg->cur_pos_item[i]);
		}
	}

	return update_minmax_idx(arg, cur_min_idx, cur_max_idx);
}

static void
posting_merge_OR(struct postmerge_arg *arg, void *extra_args)
{
	uint32_t cur_min_idx = 0;
	uint64_t cur_min;

	if (0 == update_min_idx(arg, &cur_min_idx)) {
		/* all posting lists are empty (e.g. a query that does
		 * not hit anything) */
#ifdef DEBUG_POST_MERGE
		printf("all posting lists are empty\n");
#endif
		return;
	}

	do {
		cur_min = arg->curIDs[cur_min_idx];
#ifdef DEBUG_POST_MERGE
		printf("calling post_on_merge(cur_min=%lu)\n", cur_min);
#endif
		arg->post_on_merge(cur_min, arg, extra_args);

#ifdef DEBUG_POST_MERGE
		printf("calling next_id_OR()\n");
#endif
	} while (next_id_OR(arg, &cur_min_idx));
}

static void
posting_merge_AND(struct postmerge_arg *arg, void *extra_args)
{
	uint32_t i, cur_min_idx = 0, cur_max_idx = 0;
	uint64_t cur_min;

	if (0 == update_minmax_idx(arg, &cur_min_idx, &cur_max_idx)) {
		/* all posting lists are empty (e.g. a query that does
		 * not hit anything) */
#ifdef DEBUG_POST_MERGE
		printf("all posting lists are empty\n");
#endif
		return;
	}

	do {
		cur_min = arg->curIDs[cur_min_idx];
		for (i = 0; i < arg->n_postings; i++)
			if (arg->curIDs[i] != cur_min)
				break;

#ifdef DEBUG_POST_MERGE
		printf("calling post_on_merge(cur_min=%lu)\n", cur_min);
#endif
		if (i == arg->n_postings)
			arg->post_on_merge(cur_min, arg, extra_args);

#ifdef DEBUG_POST_MERGE
		printf("calling next_id_AND()\n");
#endif
	} while (next_id_AND(arg, &cur_min_idx, &cur_max_idx));
}

bool posting_merge(struct postmerge_arg *arg, void *extra_args)
{
	uint32_t i;

	if (arg->n_postings == 0)
		return 1;

	/* initialize posting iterator */
	for (i = 0; i < arg->n_postings; i++) {
		if (arg->postings[i] != NULL &&
		    arg->post_start_fun(arg->postings[i])) {
			/* get the posting list iterator ready to return data */
#ifdef DEBUG_POST_MERGE
			printf("calling post_start_fun(post[%u])\n", i);
#endif
			arg->cur_pos_item[i] = arg->post_now_fun(arg->postings[i]);
			arg->curIDs[i] = arg->post_now_id_fun(arg->cur_pos_item[i]);
		} else {
			/* indicate this is an empty posting list (in case query
			 * term is not found)*/
#ifdef DEBUG_POST_MERGE
			printf("post[%u] := NULL\n", i);
#endif
			arg->cur_pos_item[i] = NULL;
			arg->curIDs[i] = MAX_POST_ITEM_ID;
		}
	}

	if (arg->op == POSTMERGE_OP_AND)
		posting_merge_AND(arg, extra_args);
	else if (arg->op == POSTMERGE_OP_OR)
		posting_merge_OR(arg, extra_args);
	else
		return 0;

	/* un-initialize posting iterator */
	for (i = 0; i < arg->n_postings; i++)
		if (arg->postings[i]) {
#ifdef DEBUG_POST_MERGE
			printf("post_finish_fun(post[%u])\n", i);
#endif
			arg->post_finish_fun(arg->postings[i]);
		}

	return 1;
}
