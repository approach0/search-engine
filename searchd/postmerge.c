#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "postmerge.h"

void postmerge_posts_clear(struct postmerge *pm)
{
	pm->n_postings = 0;
}

void postmerge_posts_add(struct postmerge *pm, void *post,
                         struct postmerge_callbks *calls, void *arg)
{
	pm->postings[pm->n_postings] = post;
	pm->posting_args[pm->n_postings] = arg;
	pm->curIDs[pm->n_postings] = MAX_POST_ITEM_ID;
	pm->cur_pos_item[pm->n_postings] = NULL;

	/* callback functions */
	pm->start[pm->n_postings]  = calls->start;
	pm->next[pm->n_postings]   = calls->next;
	pm->jump[pm->n_postings]   = calls->jump;
	pm->now[pm->n_postings]    = calls->now;
	pm->now_id[pm->n_postings] = calls->now_id;
	pm->finish[pm->n_postings] = calls->finish;

	pm->n_postings ++;
}

static bool
update_min_idx(struct postmerge *pm, uint32_t *cur_min_idx)
{
	uint32_t i;
	uint64_t cur_min = MAX_POST_ITEM_ID;

	for (i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] < cur_min) {
			cur_min = pm->curIDs[i];
			*cur_min_idx = i;
		}
	}

	return (cur_min != MAX_POST_ITEM_ID);
}

static bool
update_minmax_idx(struct postmerge *pm,
                  uint32_t *cur_min_idx, uint32_t *cur_max_idx)
{
	uint32_t i;
	uint64_t cur_max = 0;
	uint64_t cur_min = MAX_POST_ITEM_ID;

	for (i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] < cur_min) {
			cur_min = pm->curIDs[i];
			*cur_min_idx = i;
		}

		if (pm->curIDs[i] > cur_max) {
			cur_max = pm->curIDs[i];
			*cur_max_idx = i;
		}
	}

	return (cur_min != MAX_POST_ITEM_ID);
}

static bool
next_id_OR(struct postmerge *pm, uint32_t *cur_min_idx)
{
	uint32_t i;
	uint64_t cur_min = pm->curIDs[*cur_min_idx];

	for (i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] <= cur_min) {
			/* now, move head posting list iterator[i] */
			if (pm->next[i](pm->postings[i])) {
				/* update current pointer and ID for posting[i] */
				pm->cur_pos_item[i] = pm->now[i](pm->postings[i]);
				pm->curIDs[i] = pm->now_id[i](pm->cur_pos_item[i]);
			} else {
				/* for OR, we cannot just simply leave curIDs[i] unchanged,
				 * because in posting_merge_OR() will continue to evaluate
				 * min value.
				 */
				pm->cur_pos_item[i] = NULL;
				pm->curIDs[i] = MAX_POST_ITEM_ID;
			}
		}
	}

	return update_min_idx(pm, cur_min_idx);
}

static bool
next_id_AND(struct postmerge *pm,
        uint32_t *cur_min_idx, uint32_t *cur_max_idx)
{
	uint32_t i;
	uint64_t cur_min = pm->curIDs[*cur_min_idx];
	uint64_t cur_max = pm->curIDs[*cur_max_idx];

	for (i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] <= cur_min) {
			/* now, move head posting list iterator[i] */
			if (pm->curIDs[i] == cur_max) {
				/* in this case we do not jump() because if all
				 * curIDs[i] are equal, jump() will stuck here
				 * forever. */
				if (!pm->next[i](pm->postings[i]))
					/* iterator goes out of posting list scope
					 * i.e. curIDs[i] now is infinity, no need to
					 * go to next posting_merge_AND() iteration.
					 */
					return 0;
			} else {
				/* do jump for the sake of efficiency (although
				 * depend on jump() callback implementation) */
				if (!pm->jump[i](pm->postings[i], cur_max))
					/* similarly */
					return 0;
			}

			/* update current pointer and ID for posting[i] */
			pm->cur_pos_item[i] = pm->now[i](pm->postings[i]);
			pm->curIDs[i] = pm->now_id[i](pm->cur_pos_item[i]);
		}
	}

	return update_minmax_idx(pm, cur_min_idx, cur_max_idx);
}

static void
posting_merge_OR(struct postmerge *pm,
                  post_merge_callbk post_on_merge, void *extra_args)
{
	uint32_t cur_min_idx = 0;
	uint64_t cur_min;

	if (0 == update_min_idx(pm, &cur_min_idx)) {
		/* all posting lists are empty (e.g. a query that does
		 * not hit anything) */
#ifdef DEBUG_POST_MERGE
		printf("all posting lists are empty\n");
#endif
		return;
	}

	do {
		cur_min = pm->curIDs[cur_min_idx];
#ifdef DEBUG_POST_MERGE
		printf("calling post_on_merge(cur_min=%lu)\n", cur_min);
#endif
		post_on_merge(cur_min, pm, extra_args);

#ifdef DEBUG_POST_MERGE
		printf("calling next_id_OR()\n");
#endif
	} while (next_id_OR(pm, &cur_min_idx));
}

static void
posting_merge_AND(struct postmerge *pm,
                  post_merge_callbk post_on_merge, void *extra_args)
{
	uint32_t i, cur_min_idx = 0, cur_max_idx = 0;
	uint64_t cur_min;

	if (0 == update_minmax_idx(pm, &cur_min_idx, &cur_max_idx)) {
		/* all posting lists are empty (e.g. a query that does
		 * not hit anything) */
#ifdef DEBUG_POST_MERGE
		printf("all posting lists are empty\n");
#endif
		return;
	}

	do {
		cur_min = pm->curIDs[cur_min_idx];
		for (i = 0; i < pm->n_postings; i++)
			if (pm->curIDs[i] != cur_min)
				break;

#ifdef DEBUG_POST_MERGE
		printf("calling post_on_merge(cur_min=%lu)\n", cur_min);
#endif
		if (i == pm->n_postings)
			post_on_merge(cur_min, pm, extra_args);

#ifdef DEBUG_POST_MERGE
		printf("calling next_id_AND()\n");
#endif
	} while (next_id_AND(pm, &cur_min_idx, &cur_max_idx));
}

bool
posting_merge(struct postmerge *pm, enum postmerge_op op,
              post_merge_callbk post_on_merge, void *extra_args)
{
	uint32_t i;

	if (pm->n_postings == 0)
		return 1;

	/* initialize posting iterator */
	for (i = 0; i < pm->n_postings; i++) {
		if (pm->postings[i] != NULL &&
		    pm->start[i](pm->postings[i])) {
			/* get the posting list iterator ready to return data */
#ifdef DEBUG_POST_MERGE
			printf("calling start(post[%u])\n", i);
#endif
			pm->cur_pos_item[i] = pm->now[i](pm->postings[i]);
			pm->curIDs[i] = pm->now_id[i](pm->cur_pos_item[i]);
		} else {
			/* indicate this is an empty posting list (in case query
			 * term is not found)*/
#ifdef DEBUG_POST_MERGE
			printf("post[%u] := NULL\n", i);
#endif
			pm->cur_pos_item[i] = NULL;
			pm->curIDs[i] = MAX_POST_ITEM_ID;
		}
	}

	if (op == POSTMERGE_OP_AND)
		posting_merge_AND(pm, post_on_merge, extra_args);
	else if (op == POSTMERGE_OP_OR)
		posting_merge_OR(pm, post_on_merge, extra_args);
	else
		return 0;

	/* un-initialize posting iterator */
	for (i = 0; i < pm->n_postings; i++)
		if (pm->postings[i]) {
#ifdef DEBUG_POST_MERGE
			printf("finish(post[%u])\n", i);
#endif
			pm->finish[i](pm->postings[i]);
		}

	return 1;
}
