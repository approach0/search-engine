#pragma once
#include "config.h"
#include "bintr.h"

/* treap data structure */
struct treap_node {
	struct bintr_node bintr_nd;
	int               priority;
};

/* for random seed. Treap is a randomized data
 * structure, rand_timeseed() provide a seed
 * function to randomized treap priority value. */
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

static __inline void rand_timeseed(void)
{
	unsigned int ticks;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ticks = tv.tv_sec + tv.tv_usec;

	srand(ticks);
}

/* treap node constructor */
#define TREAP_NODE_CONS(_tn, _key) \
		BINTR_NODE_CONS(_tn.bintr_nd, _key); \
		(_tn).priority = rand()

#ifdef DEBUG_TREAP
static enum bintr_it_ret
debug_print(struct bintr_ref *ref, uint32_t level, void *arg)
{
	enum bintr_son_pos pos;
	printf("%*c[%u]", level * 4, '*', ref->this_->key);

	if(ref->this_->father) {
		pos = bintr_get_son_pos(ref->this_, ref->this_->father);
		if(pos == BINTR_LEFT)
			printf(" (left son)");
		else
			printf(" (right son)");

		printf(" (father: %u)", ref->this_->father->key);
	}

	printf("\n");
	return BINTR_IT_CONTINUE;
}
#endif

static __inline void
treap_rotate_up(struct bintr_node *me, struct bintr_node *father,
                struct bintr_node *grandpa, struct bintr_node **root)
{
	enum bintr_son_pos pos;
	enum bintr_son_pos my_abandon_pos, father_adopt_pos;
	struct bintr_node *my_abandon_son;

	/* decide rotate type */
	if (father->son[BINTR_LEFT] == me) {
		/* left rotate */
		my_abandon_pos = BINTR_RIGHT;
		father_adopt_pos = BINTR_LEFT;
	} else {
		/* right rotate */
		my_abandon_pos = BINTR_LEFT;
		father_adopt_pos = BINTR_RIGHT;
	}

#ifdef DEBUG_TREAP
	printf("before rotate:\n");
	bintr_foreach(root, &bintr_preorder,
	              &debug_print, NULL);
#endif

	/* detach myself */
	pos = bintr_get_son_pos(me, father);
	bintr_detach(me, father->son + pos);

	/* detach father from grandpa and attach me to grandpa */
	if (grandpa == NULL) {
		bintr_detach(father, root);
		bintr_attach(me, root, grandpa);
	} else {
		pos = bintr_get_son_pos(father, grandpa);

		bintr_detach(father, grandpa->son + pos);
		bintr_attach(me, grandpa->son + pos, grandpa);
	}

	/* which of my son should be passed to old father? */
	my_abandon_son = me->son[my_abandon_pos];

	if (my_abandon_son != NULL) {
		/* abandon (detach) this son */
		bintr_detach(my_abandon_son, me->son + my_abandon_pos);
		/* father adopts this son */
		bintr_attach(my_abandon_son, father->son + father_adopt_pos, father);
	}

	/* attach old father to me */
	bintr_attach(father, me->son + my_abandon_pos, me);

#ifdef DEBUG_TREAP
	printf("after rotate:\n");
	bintr_foreach(root, &bintr_preorder,
	              &debug_print, NULL);
	{ char c; scanf("%c", &c); }
#endif
}

static __inline struct treap_node*
treap_insert(struct treap_node **root_, struct treap_node *new__)
{
	struct bintr_node **root = (struct bintr_node **)root_;
	struct bintr_node *inserted, *new_ = &new__->bintr_nd;
	struct treap_node *father;

	inserted = bintr_insert(root, new_);

	if (inserted == NULL /* not inserted */)
		return NULL;

	if (new_->father == NULL /* no room for rotating up */)
		return new__;

	do {
		father = MEMBER_2_STRUCT(new_->father, struct treap_node, bintr_nd);

		if (new__->priority > father->priority)
			treap_rotate_up(new_, new_->father, new_->father->father, root);
		else
			break;

	} while (new_->father);

	return new__;
}

static __inline bool
treap_rotate_down(struct bintr_node **root, struct bintr_node *this_)
{
	struct bintr_node *son;

	/* rotate one of the children up, so `this_' will go down one level */
	son = this_->son[BINTR_LEFT];
	if (son) {
		treap_rotate_up(son, this_, this_->father, root);
		return 1;
	}

	son = this_->son[BINTR_RIGHT];
	if (son) {
		treap_rotate_up(son, this_, this_->father, root);
		return 1;
	}

	return 0;
}

static __inline struct treap_node*
treap_detach(struct treap_node **root_, bintr_key_t key)
{
	struct bintr_node **root = (struct bintr_node **)root_;
	struct bintr_node  *father;
	struct bintr_ref    ref;
	enum bintr_son_pos  pos;

	/* search the node to be removed */
	ref = bintr_find(root, key);
	if (ref.this_ == NULL)
		return NULL;

	/* rotate down to bottom */
	while (treap_rotate_down(root, ref.this_));

	/* finally detach it */
	father = ref.this_->father;
	if (father) {
		pos = bintr_get_son_pos(ref.this_, father);
		bintr_detach(ref.this_, father->son + pos);
	} else {
		bintr_detach(ref.this_, root);
	}

	return MEMBER_2_STRUCT(ref.this_, struct treap_node, bintr_nd);
}


static __inline struct treap_node*
treap_find(struct treap_node *root, bintr_key_t key)
{
	struct bintr_node *bintr_root = &root->bintr_nd;
	struct bintr_ref ref;
	ref = bintr_find(&bintr_root, key);

	if (ref.this_ == NULL) {
		return NULL;
	} else {
		return MEMBER_2_STRUCT(ref.this_, struct treap_node, bintr_nd);
	}
}
