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

struct list_iterator list_generator(list li)
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

int main()
{
	list li = LIST_NULL;
	struct T *p;
	int i;

	/* initialize list */
	for (i = 0; i < 9; i++) {
		p = malloc(sizeof(struct T));
		p->i = i;
		LIST_NODE_CONS(p->ln);
		list_insert_one_at_tail(&p->ln, &li, NULL, NULL);
	}

	/* generic for */
	{
		struct list_iterator g = list_generator(li);
		do {
			struct T *t = MEMBER_2_STRUCT(g.cur.now, struct T, ln);
			printf("%d\n", t->i);
		} while (list_next(li, &g));
	}

	/* generic for */
	{
		struct list_iterator g = list_generator(li);
		do {
			struct T *t = MEMBER_2_STRUCT(g.cur.now, struct T, ln);
			printf("%d\n", t->i);
		} while (list_next(li, &g));
	}

	/* generic for */
	{
		struct list_iterator g = list_generator(li);
		do {
			struct T *t = MEMBER_2_STRUCT(g.cur.now, struct T, ln);
			printf("- %d\n", t->i);

			/* generic for */
			{
				struct list_iterator g = list_generator(li);
				do {
					struct T *t = MEMBER_2_STRUCT(g.cur.now, struct T, ln);
					printf("-- %d\n", t->i);
				} while (list_next(li, &g));
			}
		} while (list_next(li, &g));
	}

	// for_each(node, list, li) {
	// 	// node = g.cur.now;
	// 	struct T *t = MEMBER_2_STRUCT(node, struct T, ln);
	// 	printf("%d\n", t->i);
	// }
	
	/* release memory */
	list_release(&li);
	mhook_print_unfree();
	return 0;
}
