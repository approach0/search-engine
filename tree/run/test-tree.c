#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "tree.h"

struct T {
	int i;
	struct tree_node tnd;
};

/* some thing we need to print the tree nicely */ 
static int depth_flag[64];
#define DEPTH_END        0
#define DEPTH_BEGIN      1
#define DEPTH_GOING_END  2

static
TREE_IT_CALLBK(easy_print)
{
	TREE_OBJ(struct T, p, tnd);
	printf("easy print: %d \n", p->i);
	LIST_GO_OVER;
}

static
TREE_IT_CALLBK(pretty_print)
{
	TREE_OBJ(struct T, p, tnd);
	int i;

	if (pa_now->now == pa_head->last)
		depth_flag[pa_depth] = DEPTH_GOING_END;
	else if (pa_now->now == pa_head->now)
		depth_flag[pa_depth] = DEPTH_BEGIN;

	for (i = 0; i<pa_depth; i++) {
		switch (depth_flag[i + 1]) {
		case DEPTH_END:
			printf("   ");
			break;
		case DEPTH_BEGIN:
			printf("  |");
			break;
		case DEPTH_GOING_END:
			printf("  └");
			break;
		default:
			break;
		}
	}
	
	printf("──%d \n", p->i);
	
	if (depth_flag[pa_depth] == DEPTH_GOING_END)
		depth_flag[pa_depth] = DEPTH_END;

	LIST_GO_OVER;
}

/* pluck an 8 and plug it under branch 5 */
static
TREE_IT_CALLBK(pluck_and_plug)
{
	TREE_OBJ(struct T, p, tnd);
	P_CAST(pluck, struct T *, pa_extra);

	if (p->i == 8) {
		if (!(*pluck)) {
			*pluck = p;
			return tree_detach(&p->tnd, pa_now, pa_fwd);
		}
	}

	if (p->i == 5) {
		if (*pluck) {
			tree_attach(&(*pluck)->tnd, &p->tnd, pa_now, pa_fwd);
			return LIST_RET_BREAK;
		}
	}

	LIST_GO_OVER;
}

/* release resource callback function */
static
TREE_IT_CALLBK(release)
{
	TREE_OBJ(struct T, p, tnd);
	bool res;

	printf("free %d \n", p->i);
	res = tree_detach(&p->tnd, pa_now, pa_fwd);
	/* Or you can use: 
	 * res = list_detach_one(pa_now->now, pa_head, pa_now, pa_fwd);
	 * This however, will not set father of each to NULL, but this
	 * really doesn't matter because you are releasing the tree.
	 */
	free(p);

	return res;
}

#define ATTACH(_i, _father) \
	p[_i] = malloc(sizeof(struct T)); \
	p[_i]->i = _i; \
	TREE_NODE_CONS(p[_i]->tnd); \
	tree_attach(&p[_i]->tnd, _father, NULL, NULL)

int 
main()
{
	struct T root, *p[60], *to_pluck = NULL;
	/* make a root */
	root.i = 0;
	TREE_NODE_CONS(root.tnd);

	/* then add nodes to the tree */
	ATTACH(1, &root.tnd);
	ATTACH(2, &root.tnd);
	ATTACH(3, &root.tnd);
	ATTACH(4, &p[2]->tnd);
	ATTACH(5, &p[2]->tnd);
	ATTACH(6, &p[4]->tnd);
	ATTACH(7, &p[4]->tnd);
	ATTACH(8, &p[4]->tnd);
	ATTACH(9, &p[5]->tnd);
	ATTACH(10, &p[5]->tnd);
	ATTACH(11, &p[5]->tnd);
	ATTACH(12, &p[1]->tnd);
	ATTACH(13, &p[1]->tnd);
	ATTACH(14, &p[12]->tnd);
	ATTACH(15, &p[3]->tnd);
	
	/* print the tree */
	printf("whole tree:\n");
	tree_foreach(&root.tnd, &tree_pre_order_DFS, &pretty_print, 
	             0, NULL);
	
	/* print a sub-tree */
	printf("a subtree:\n");
	tree_foreach(&p[2]->tnd, &tree_pre_order_DFS, &pretty_print, 
	             0, NULL);
	
	/* print a sub-tree */
	printf("a subtree in easy print:\n");
	tree_foreach(&p[2]->tnd, &tree_post_order_DFS, &easy_print, 
	             0, NULL);
	
	/* pluck and plug */
	tree_foreach(&root.tnd, &tree_pre_order_DFS, 
	             &pluck_and_plug, 0, &to_pluck);
	
	/* print the tree again to see the difference */
	printf("after pluck and plug:\n");
	tree_foreach(&root.tnd, &tree_pre_order_DFS, &pretty_print, 
	             0, NULL);

	/* release the tree 
	 * Please note when releasing the tree, you should use
	 * tree_post_order_DFS way to traversal. Plus, here our 
	 * root is not dynamically allocated, so pass 1 to the 4th
	 * parameter to avoid freeing the root.
	 */
	tree_foreach(&root.tnd, &tree_post_order_DFS, &release, 
	             1, NULL);

	mhook_print_unfree();
	exit(0);
}
