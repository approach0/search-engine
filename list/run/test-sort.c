#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

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
	printf("%d--", p->i);
	LIST_GO_OVER;
}

static
LIST_CMP_CALLBK(compare)
{
	struct T *p0 = MEMBER_2_STRUCT(pa_node0, struct T, ln);
	struct T *p1 = MEMBER_2_STRUCT(pa_node1, struct T, ln);
	P_CAST(extra, int, pa_extra);

	assert(*extra == 123); (void)extra;
	return p0->i > p1->i;
}

LIST_DEF_FREE_FUN(list_release, struct T, ln, free(p));
LIST_DEF_FREE_FUN(list_release2, struct T, ln, free(p));

/* print list */
#define PRINT_LIST \
	list_foreach(&list, &print, NULL); \
	printf("NULL \n")

int
main()
{
	struct list_it list = LIST_NULL;
	struct T *p;
	struct list_sort_arg sort;
	int i;
	int extra = 123;

	srand(time(NULL));

	/* insert into list some entries orderly */
	for (i=0; i<100; i++) {
		p = malloc(sizeof(struct T));
		p->i = rand() % 100;
		LIST_NODE_CONS(p->ln);
		list_insert_one_at_tail(&p->ln, &list, NULL, NULL);
	}
	PRINT_LIST;

	/* sort list */
	sort.cmp = &compare;
	sort.extra = &extra;
	list_sort(&list, &sort);
	PRINT_LIST;

	/* free list */
	list_release(&list);

	/* test for sort insert */
	for (i=0; i<9; i++) {
		p = malloc(sizeof(struct T));
		p->i = i % 3 + 2;
		LIST_NODE_CONS(p->ln);
		list_sort_insert(&p->ln, &list, &sort);
	}
	PRINT_LIST;

	/* free list */
	list_release(&list);

	mhook_print_unfree();
	return 0;
}
