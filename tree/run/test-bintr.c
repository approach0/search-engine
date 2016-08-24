#include <assert.h>
#include <stdio.h>

/* for random generation */
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* binary tree header */
#include "list/list.h"
#include "bintr.h"

#include "mhook/mhook.h"

static void rank_seed(void)
{
	unsigned int ticks;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ticks = tv.tv_sec + tv.tv_usec;

	srand(ticks);
}

struct T {
	int data;
	struct bintr_node bintr_nd;
};

static enum bintr_it_ret
print_T(struct bintr_ref *ref, uint32_t level, void *arg)
{
	P_CAST(tab, int, arg);
	struct T *t = MEMBER_2_STRUCT(ref->this_, struct T, bintr_nd);

	if (*tab)
		printf("%*c[%u]:%d", level * 4, '*', ref->this_->key, t->data);
	else
		printf("%c[%u]:%d", '*', ref->this_->key, t->data);

	if (t->bintr_nd.father != NULL)
		printf(" (father: %u)", ref->father->key);

	printf("\n");
	return BINTR_IT_CONTINUE;
}

static enum bintr_it_ret
free_T(struct bintr_ref *ref, uint32_t level, void *arg)
{
	struct T *t = MEMBER_2_STRUCT(ref->this_, struct T, bintr_nd);

	printf("free node...\n");
	bintr_detach(ref->this_, ref->ptr_to_this);
	free(t);

	return BINTR_IT_CONTINUE;
}

int main()
{
	int i, tab;
	struct bintr_node *root = NULL;
	struct bintr_node *inserted;
	bintr_key_t        key;
	struct T          *new;

	rank_seed();

	for (i = 0; i < 10; i++) {
		new = malloc(sizeof(struct T));
		key = rand() % 10;
		BINTR_NODE_CONS(new->bintr_nd, key);
		new->data = key * key;

		inserted = bintr_insert(&root, &new->bintr_nd);
		if (inserted == NULL)
			free(new);
		else
			printf("inserted.\n");
	}

	printf("pre-order print:\n");
	tab = 1;
	bintr_foreach(&root, &bintr_preorder, &print_T, &tab);

	printf("in-order print:\n");
	tab = 0;
	bintr_foreach(&root, &bintr_inorder, &print_T, &tab);

	printf("in-order descending print:\n");
	tab = 0;
	bintr_foreach(&root, &bintr_inorder_desc, &print_T, &tab);

	printf("post-order free:\n");
	bintr_foreach(&root, &bintr_postorder, &free_T, NULL);

	printf("root ptr = %p\n", root);
	assert(root == NULL);

	mhook_print_unfree();
	return 0;
}
