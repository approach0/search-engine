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
	uint32_t cur_num, jump_num;

	skippy_init(&skippy, 3);
	skippy_print(&skippy);

	for (i = 0; i < 30; i++) {
		t = malloc(sizeof(struct T));
		t->data = i * i;
		skippy_node_init(&t->sn, i * 2);
		skippy_append(&skippy, &t->sn);
	}

	skippy_print(&skippy);

	printf("Please input 'cur_num, jump_num:'\n");
	scanf("%u,%u", &cur_num, &jump_num);

	skippy_foreach(cur, save, &skippy, 0) {
		if (cur->key == cur_num)
			break;
	}

	if (cur != NULL) {
		skippy_node_print(cur);
		res = skippy_node_jump(cur, jump_num);

		if (res)
			printf("reach %u\n", res->key);
		else
			printf("reach the end.\n");
	} else {
		printf("cannot find node key %u\n", cur_num);
	}

	skippy_free(&skippy, struct T, sn, free(p));
	mhook_print_unfree();
	return 0;
}
