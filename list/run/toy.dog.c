#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "list.h"

struct T
{
	int i;
	struct list_node ln;
};

LIST_DEF_FREE_FUN(list_release, struct T, ln, free(p));

struct list_iterator {
	struct list_it fwd, cur;
};

struct list_iterator list_mk_iterator(list li)
{
	struct list_iterator g;
	g.cur = list_get_it(li.now);
	g.fwd = list_get_it(li.now->next);
	return g;
}

int list_next(list li, struct list_iterator *g)
{
	g->cur = g->fwd;
	g->fwd = list_get_it(g->cur.now->next);

	if (li.now == g->cur.now) {
		return 0;
	} else {
		return 1;
	}
}

#define LIST_ITEM(_item, _type, _iter) \
	_type *_item = MEMBER_2_STRUCT(_iter.cur.now, _type, ln)

int main()
{
	list li = LIST_NULL;
	struct T *p;
	int i;

	/* initialize list */
	for (i = 0; i < 9; i++) {
		p = malloc(sizeof(struct T));
		p->i = (i * 2) %3;
		LIST_NODE_CONS(p->ln);
		list_insert_one_at_tail(&p->ln, &li, NULL, NULL);
	}

	foreach (iter1, list, li) {
		LIST_ITEM(t1, struct T, iter1);
		int i = 0;
		foreach (iter2, list, li) {
			LIST_ITEM(t2, struct T, iter2);
			printf("%d -- [%d]: %d\n", t1->i, i++, t2->i);
		}
	}
	
	/* release memory */
	list_release(&li);
	mhook_print_unfree();
	return 0;
}
