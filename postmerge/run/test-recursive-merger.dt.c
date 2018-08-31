#include <stdio.h>
#include "mhook/mhook.h"
#include "common/common.h"
#include "postmerger.h"

/*
 * Path posting list
 */

struct path_postlist {
	uint32_t docID[128];
	uint32_t expID[128];
	uint64_t len, idx;
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

struct postmerger_postlist
postmerger_path_postlist(struct path_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		&path_postlist_cur,
		&path_postlist_next,
		NULL,
		NULL
	};
	return ret;
}

/*
 * Top-level recursive posting list
 */

struct recur_postlist {
	struct postmerger pm;
	postmerger_iter_t iter;
};

int recur_postlist_next(void *po_)
{
	PTR_CAST(po, struct recur_postlist, po_);
	uint32_t prev_docID = (uint32_t)po->iter->min;
	do {
		uint32_t min_docID = (uint32_t)po->iter->min;
		if (prev_docID != min_docID)
			return 1;
		else
			prev_docID = min_docID;

		for (int i = 0; i < po->iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
			uint32_t docID = (uint32_t)(cur >> 0);
			uint32_t expID = (uint32_t)(cur >> 32);

			printf("sub-post[%u]: %u, %u \n", po->iter->map[i], docID, expID);
			if (cur == po->iter->min)
				postmerger_iter_call(&po->pm, po->iter, next, i);
		}
		printf("\n");

	} while (postmerger_iter_next(po->iter));

	return 0;
}

uint64_t recur_postlist_cur(void *po_)
{
	PTR_CAST(po, struct recur_postlist, po_);
	if (po->iter->min == UINT64_MAX) {
		return UINT64_MAX;
	} else {
		uint32_t min_docID = (uint32_t)po->iter->min;
		return min_docID;
	}
}

struct postmerger_postlist
postmerger_recur_postlist(struct recur_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		&recur_postlist_cur,
		&recur_postlist_next
	};
	return ret;
}

/*
 * Example
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

	struct recur_postlist math1; /* will associate a tag and boolean_op */
	postmerger_init(&math1.pm);
	math1.pm.po[math1.pm.n_po ++] = postmerger_path_postlist(&math1_path1);
	math1.pm.po[math1.pm.n_po ++] = postmerger_path_postlist(&math1_path2);
	math1.iter = postmerger_iterator(&math1.pm);

	struct postmerger root_pm;
	postmerger_init(&root_pm);
	root_pm.po[root_pm.n_po ++] = postmerger_recur_postlist(&math1);

	foreach (iter, postmerger, &root_pm) {
		for (int i = 0; i < iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&root_pm, iter, cur, i);

			printf("top-post[%u]: %lu \n", iter->map[i], cur);
			if (cur == iter->min)
				postmerger_iter_call(&root_pm, iter, next, i);
		}
		printf("\n");
	}

	postmerger_iter_free(math1.iter);
	mhook_print_unfree();
	return 0;
}
