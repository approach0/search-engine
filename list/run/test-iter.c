#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "list.h"

struct T
{
	int i;
	struct list_node ln;
};

static
LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct T, p, ln);

	if (pa_now->now == pa_head->now) {
		printf("first: %d\n", p->i);
	}
	
	if (pa_now->now == pa_head->last) {
		printf("last: %d\n", p->i);
		return LIST_RET_BREAK;

	} else {
		return LIST_RET_CONTINUE;
	}
}

LIST_DEF_FREE_FUN(list_release, struct T, ln, free(p));

int main()
{
	list li = LIST_NULL;
	struct T *p;
	int i;

	for (i = 0; i < 9; i++) {
		p = malloc(sizeof(struct T));
		p->i = i;
		LIST_NODE_CONS(p->ln);
		list_insert_one_at_tail(&p->ln, &li, NULL, NULL);
	}
	
	list_foreach(&li, &print, NULL);
	printf("NULL \n");

	list_release(&li);

	mhook_print_unfree();
	return 0;
}
