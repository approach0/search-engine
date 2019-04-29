#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mhook/mhook.h"
#include "common/common.h"
#include "postmerger.h"

/*
 * Low-level path posting list
 */
struct path_postlist {
	uint32_t docID[128];
	uint32_t expID[128];
	uint32_t len, idx;
};

int path_postlist_next(void *po_)
{
	PTR_CAST(po, struct path_postlist, po_);
	if (po->idx < po->len) {
		po->idx += 1;
		return 1;
	} else {
		return 0;
	}
}

uint64_t path_postlist_cur(void *po_)
{
	PTR_CAST(po, struct path_postlist, po_);
	if (po->idx < po->len) {
		uint64_t compactID = po->expID[po->idx];
		compactID = compactID << 32;
		compactID |= po->docID[po->idx];
		return compactID;
	} else {
		return UINT64_MAX;
	}
}

static void *path_postlist_get_iter(void *po)
{
	return po;
}

static void path_postlist_del_iter(void *po)
{
	return;
}

struct postmerger_postlist
path_postlist(struct path_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		&path_postlist_cur,
		&path_postlist_next,
		NULL /* jump */,
		NULL /* read */,
		path_postlist_get_iter,
		path_postlist_del_iter
	};
	return ret;
}

/*
 * Top-level recursive posting list
 */
int recur_postlist_next(void *iter_)
{
	PTR_CAST(iter, struct postmerger, iter_);
	uint32_t prev_docID = (uint32_t)iter->min;
	do {
		uint32_t min_docID = (uint32_t)iter->min;
		if (prev_docID != min_docID)
			return 1;
		else
			prev_docID = min_docID;

		for (int i = 0; i < iter->size; i++) {
			uint64_t cur = postmerger_iter_call(iter, cur, i);
			uint32_t docID = (uint32_t)(cur >> 0);
			uint32_t expID = (uint32_t)(cur >> 32);

			printf("sub-post[%u]: %u, %u \n", iter->map[i], docID, expID);
			if (cur == iter->min)
				postmerger_iter_call(iter, next, i);
		}
		printf("\n");

	} while (postmerger_iter_next(iter));

	return 0;
}

uint64_t recur_postlist_cur(void *iter_)
{
	PTR_CAST(iter, struct postmerger, iter_);
	if (iter->min == UINT64_MAX) {
		return UINT64_MAX;
	} else {
		uint32_t min_docID = (uint32_t)iter->min;
		return min_docID;
	}
}

static void *recur_postlist_get_iter(void *low_pols)
{
	return postmerger_iterator(low_pols);
}

static void recur_postlist_del_iter(void *iter)
{
	postmerger_iter_free(iter);
}

struct postmerger_postlist
recur_postlist(void *po)
{
	struct postmerger_postlist ret = {
		po,
		&recur_postlist_cur,
		&recur_postlist_next,
		NULL /* jump */,
		NULL /* read */,
		recur_postlist_get_iter,
		recur_postlist_del_iter
	};
	return ret;
}

/*
 * Test
 */
int main()
{
	struct path_postlist math1_path1 = {
		{1, 1, 1, 2},
		{1, 2, 3, 1},
	4, 0};

	struct path_postlist math1_path2 = {
		{1, 1, 2, 2},
		{2, 3, 1, 2},
	4, 0};

	struct postmerger_postlists low_pols = {0};
	low_pols.po[low_pols.n ++] = path_postlist(&math1_path1);
	low_pols.po[low_pols.n ++] = path_postlist(&math1_path2);

	struct postmerger_postlists root_pols = {0};
	root_pols.po[root_pols.n ++] = recur_postlist(&low_pols);

	foreach (iter, postmerger, &root_pols) {
		for (int i = 0; i < iter->size; i++) {
			uint64_t cur = postmerger_iter_call(iter, cur, i);
			printf("top-post[%u]: %u\n", iter->map[i], (uint32_t)cur);

			if (cur == iter->min)
				postmerger_iter_call(iter, next, i);
		}
		printf("\n");
	}

	mhook_print_unfree();
	return 0;
}
