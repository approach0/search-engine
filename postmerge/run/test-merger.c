#include <stdio.h>
#include "common/common.h"
#include "postmerge/config.h"

struct simple_postlist {
	uint64_t posts[128];
	uint64_t len, idx;
};

int simple_postlist_next(void *po_)
{
	PTR_CAST(po, struct simple_postlist, po_);
	if (po->idx < po->len) {
		po->idx += 1;
		return 0;
	} else {
		return 1;
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

/* ================================ */
typedef uint64_t (*postmerger_callbk_cur)(void *);
typedef int      (*postmerger_callbk_next)(void *);

struct postmerger_postlist {
	void                   *po;
	postmerger_callbk_cur   cur;
	postmerger_callbk_next  next;
};

struct postmerger_postlist
postmerger_simple_postlist(struct simple_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		&simple_postlist_cur,
		&simple_postlist_next,
	};
	return ret;
}

struct postmerger {
	struct postmerger_postlist po[MAX_MERGE_POSTINGS];
	int n_po;
	int iter_n;
	int iter_map[MAX_MERGE_POSTINGS];
};

void postmerger_init(struct postmerger *pm)
{
	pm->n_po = 0;
}

void postmerger_iter_reset(struct postmerger *pm)
{
	pm->iter_n = pm->n_po;
	for (int i = 0; i < pm->iter_n; i++) {
		pm->iter_map[i] = i;
	}
}

void postmerger_iter_remove(struct postmerger *pm, int i)
{
	for (int j = i; j < pm->iter_n - 1; j++) {
		pm->iter_map[j] = pm->iter_map[j + 1];
	}
	pm->iter_n -= 1;
}

#define POSTMERGER_POSTLIST_CALL(_pm, _fun, _i, ...) \
	(*((_pm)->po[_i]._fun)) ((_pm)->po[_i].po, ##__VA_ARGS__)

#define POSTMERGER_ITER_CALL(_pm, _fun, _i, ...) \
	POSTMERGER_POSTLIST_CALL(_pm, _fun, (_pm)->iter_map[_i])

int main()
{
	struct simple_postlist a = {{3, 4, 6, 7, 8, 9, 11, 12, 14}, 9, 0};
	struct simple_postlist b = {{3, 5, 6, 7, 9, 11}, 6, 0};
	struct simple_postlist c = {{4, 6, 7, 9, 11, 12}, 6, 0};

	struct postmerger pm;
	postmerger_init(&pm);
	pm.po[pm.n_po ++] = postmerger_simple_postlist(&a);
	pm.po[pm.n_po ++] = postmerger_simple_postlist(&b);
	pm.po[pm.n_po ++] = postmerger_simple_postlist(&c);
	postmerger_iter_reset(&pm);

	uint64_t min;
	do {
		min = UINT64_MAX;
		for (int i = 0; i < pm.iter_n; i++) {
			uint64_t cur = POSTMERGER_ITER_CALL(&pm, cur, i);
			if (cur < min) min = cur;
		}

		{
			for (int i = 0; i < pm.iter_n; i++) {
				uint64_t cur = POSTMERGER_ITER_CALL(&pm, cur, i);

				if (cur == UINT64_MAX) {
					printf("removing [%d]\n", pm.iter_map[i]);
					postmerger_iter_remove(&pm, i);
					i --;
					continue;
				} else {
					printf("%u: %lu \n", pm.iter_map[i], cur);

					if (cur == min)
						POSTMERGER_ITER_CALL(&pm, next, i);
				}
			}
			printf("\n");
		}
	} while (min != UINT64_MAX);

	return 0;
}
