#include <stdio.h>
#include "mhook/mhook.h"
#include "common/common.h"
#include "postmerger.h"

struct simple_postlist {
	uint64_t posts[128];
	uint64_t len, idx;
};

int simple_postlist_next(void *po_)
{
	PTR_CAST(po, struct simple_postlist, po_);
	if (po->idx < po->len) {
		po->idx += 1;
		return 1;
	} else {
		return 0;
	}
}

uint64_t simple_postlist_cur(void *po_)
{
	PTR_CAST(po, struct simple_postlist, po_);
	if (po->idx < po->len) {
		return po->posts[po->idx];
	} else {
		return UINT64_MAX;
	}
}

static void *simple_postlist_get_iter(void *po)
{
	return po;
}

static void simple_postlist_del_iter(void *po)
{
	return;
}

struct postmerger_postlist
postmerger_simple_postlist(struct simple_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		&simple_postlist_cur,
		&simple_postlist_next,
		NULL /* jump */,
		NULL /* read */,
		simple_postlist_get_iter,
		simple_postlist_del_iter
	};
	return ret;
}

int main()
{
	struct simple_postlist a = {{3, 4, 6, 7, 8, 9, 11, 12, 14}, 9, 0};
	struct simple_postlist b = {{3, 5, 6, 7, 9, 11}, 6, 0};
	struct simple_postlist c = {{4, 6, 7, 9, 11, 12}, 6, 0};

	struct postmerger_postlists pols = {0};
	pols.po[pols.n ++] = postmerger_simple_postlist(&a);
	pols.po[pols.n ++] = postmerger_simple_postlist(&b);
	pols.po[pols.n ++] = postmerger_simple_postlist(&c);
	
	foreach (iter, postmerger, &pols) {
		for (int i = 0; i < iter->size; i++) {
			uint64_t cur = postmerger_iter_call(iter, cur, i);

			if (cur == UINT64_MAX) {
				printf("removing [%d]\n", iter->map[i]);
				postmerger_iter_remove(iter, i);
				i -= 1; /* repeat position[i] next loop */
				continue;
			} else {
				printf("%u: %lu \n", iter->map[i], cur);
				if (cur == iter->min)
					postmerger_iter_call(iter, next, i);
			}
		}
		printf("\n");
	}

	mhook_print_unfree();
	return 0;
}
