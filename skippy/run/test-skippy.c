#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"

#define DEBUG_SKIPPY
#include "skippy.h"

struct T {
	int data;
	struct skippy_node sn;
};

int main(void)
{
	int i;
	struct T *t;
	struct skippy skippy;
	struct skippy_node *res = NULL;

	struct skippy_node *cur, *save;
	uint64_t cur_num, jump_num;

	skippy_init(&skippy, 3);
	skippy_print(&skippy, true);

	for (i = 0; i < 30; i++) {
		t = malloc(sizeof(struct T));
		t->data = i * i;
		skippy_node_init(&t->sn, i * 2);
		skippy_append(&skippy, &t->sn);
	}

	skippy_print(&skippy, true);

	printf("Please input 'cur_num, jump_num:'\n");
	(void)scanf("%lu,%lu", &cur_num, &jump_num);

	skippy_foreach(cur, save, &skippy, 0) {
		if (cur->key == cur_num)
			break;
	}

	if (cur != NULL) {
		skippy_node_print(cur);
		//res = skippy_node_jump(cur, jump_num);
		res = skippy_node_lazy_jump(cur, jump_num);
		printf("reach %lu\n", res->key);

		if (res->key < jump_num)
			printf("reach the end.\n");
	} else {
		printf("cannot find start node (key=%lu)\n", cur_num);
	}

	skippy_free(&skippy, struct T, sn, free(p));
	mhook_print_unfree();
	return 0;
}
