#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

enum bintr_son_pos {
	BINTR_LEFT,
	BINTR_RIGHT
};

typedef uint32_t bintr_key_t;

/* binary tree data structure */
struct bintr_node {
	struct bintr_node *son[2];
	struct bintr_node *father;
	bintr_key_t        key;
};

/* binary tree node constructor */
#define BINTR_NODE_CONS(_tn, _key) \
		(_tn).son[0] = NULL; \
		(_tn).son[1] = NULL; \
		(_tn).father = NULL; \
		(_tn).key = _key

enum bintr_it_ret {
	BINTR_IT_STOP,
	BINTR_IT_CONTINUE
};

struct bintr_ref {
	struct bintr_node *father, **ptr_to_this, *this_;
};

typedef enum bintr_it_ret
(*bintr_it_callbk)(struct bintr_ref*, uint32_t level, void *arg);

typedef void
(*bintr_trav_callbk)(struct bintr_ref*, uint32_t, bintr_it_callbk, void*);

static __inline int bintr_cmpkey(bintr_key_t key1, bintr_key_t key2)
{
	if (key1 > key2)
		return 1;
	else if (key1 < key2)
		return -1;
	else
		return 0;
}

static __inline enum bintr_son_pos
bintr_get_son_pos(struct bintr_node *son, struct bintr_node *father)
{
	if (father->son[BINTR_LEFT] == son)
		return BINTR_LEFT;
	else
		return BINTR_RIGHT;
}

static __inline struct bintr_ref
bintr_find(struct bintr_node **root, bintr_key_t key)
{
	struct bintr_ref ref = {NULL, root, *root};
	int diff;

	while (ref.this_ && (diff = bintr_cmpkey(ref.this_->key, key))) {
		enum bintr_son_pos pos = (diff > 0) ? BINTR_LEFT : BINTR_RIGHT;

		/* iterate to the choosen son, update `ref' */
		ref.father = ref.this_;
		ref.ptr_to_this = ref.this_->son + pos;
		ref.this_ = ref.this_->son[pos]; /* can be NULL */
	}

	return ref;
}

static __inline void
bintr_attach(struct bintr_node *this_, struct bintr_node **ptr_to_this,
             struct bintr_node *father)
{
	*ptr_to_this = this_;
	this_->father = father;
}

static __inline void
bintr_detach(struct bintr_node *this_, struct bintr_node **ptr_to_this)
{
	*ptr_to_this = NULL;
	this_->father = NULL;
}

static __inline struct bintr_node*
bintr_insert(struct bintr_node **root, struct bintr_node *new_)
{
	struct bintr_ref ref;
	ref = bintr_find(root, new_->key);

	if (ref.this_ == NULL) {
		/* insert at the suggested position */
		bintr_attach(new_, ref.ptr_to_this, ref.father);
		return new_;
	}

	/* same key found, do not insert */
	return NULL;
}

/* Traversal callbacks */
void bintr_preorder(struct bintr_ref*, uint32_t, bintr_it_callbk, void*);
void bintr_inorder(struct bintr_ref*, uint32_t, bintr_it_callbk, void*);
void bintr_inorder_desc(struct bintr_ref*, uint32_t, bintr_it_callbk, void*);
void bintr_postorder(struct bintr_ref*, uint32_t, bintr_it_callbk, void*);

/* Traversal function */
static __inline void
bintr_foreach(struct bintr_node **root, bintr_trav_callbk trav_fun,
              bintr_it_callbk it_fun, void *arg)
{
	struct bintr_ref ref = {NULL, root, *root};

	if (*root == NULL)
		return;

	trav_fun(&ref, 0, it_fun, arg);
}
