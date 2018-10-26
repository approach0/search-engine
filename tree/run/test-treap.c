#include <assert.h>
#include <stdio.h>

#include "mhook/mhook.h"

#include "list/list.h"
#include "treap.h"

struct T {
	int data;
	struct treap_node trp_nd;
};

static enum bintr_it_ret
print_T(struct bintr_ref *ref, uint32_t level, void *arg)
{
	P_CAST(tab, int, arg);
	struct T *t = MEMBER_2_STRUCT(ref->this_, struct T, trp_nd.bintr_nd);
	struct treap_node *trp = &t->trp_nd;
	struct bintr_node *btr = &trp->bintr_nd;

	if (*tab)
		printf("%*c[%u]:%d", level * 4, '*', btr->key, t->data);
	else
		printf("%c[%u]:%d", '*', btr->key, t->data);

	printf(" (priority: %d)", trp->priority);

	if (btr->father != NULL)
		printf(" (father: %u)", ref->father->key);

	printf("\n");
	return BINTR_IT_CONTINUE;
}

static enum bintr_it_ret
free_T(struct bintr_ref *ref, uint32_t level, void *arg)
{
	struct T *t = MEMBER_2_STRUCT(ref->this_, struct T, trp_nd.bintr_nd);

	printf("free node...\n");
	bintr_detach(ref->this_, ref->ptr_to_this);
	free(t);

	return BINTR_IT_CONTINUE;
}

int main()
{
	int i, tab;
	struct treap_node *root = NULL;
	struct treap_node *inserted, *detached;
	bintr_key_t        key;
	struct T          *new, *dele;

	rand_timeseed();

	for (i = 0; i < 20; i++) {
		new = malloc(sizeof(struct T));
		key = i % 10;
		TREAP_NODE_CONS(new->trp_nd, key);
		new->data = key * key;

		inserted = treap_insert(&root, &new->trp_nd);
		if (inserted == NULL)
			free(new);
		else
			printf("inserted.\n");
	}

	do {
		printf("pre-order print:\n");
		tab = 1;
		bintr_foreach((struct bintr_node **)&root, &bintr_preorder,
				&print_T, &tab);

		printf("in-order print:\n");
		tab = 0;
		bintr_foreach((struct bintr_node **)&root, &bintr_inorder,
				&print_T, &tab);

		printf("Please input a [key] value to remove:\n");
		(void)scanf("%u", &key);
		if (NULL != (detached = treap_detach(&root, key))) {
			dele = MEMBER_2_STRUCT(detached, struct T, trp_nd);
			printf("free detached [%u]:%u (priority: %u).\n",
			       key, dele->data, detached->priority);
			free(dele);
		}

	} while (key != 0);

	printf("post-order free:\n");
	bintr_foreach((struct bintr_node **)&root, &bintr_postorder,
	              &free_T, NULL);

	printf("root ptr = %p\n", root);
	assert(root == NULL);

	mhook_print_unfree();
	return 0;
}
