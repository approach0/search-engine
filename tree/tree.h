#pragma once
#include "list/list.h"

struct tree_node {
	struct tree_node *father;
	struct list_node ln;
	struct list_it sons;
};

/* tree node constructor */
#define TREE_NODE_CONS(_tn) \
		LIST_NODE_CONS((_tn).ln); \
		LIST_CONS((_tn).sons); \
		(_tn).father = NULL

/* attach a tree to another tree */
static __inline void 
tree_attach(struct tree_node *node, 
		struct tree_node *to,
		struct list_it   *now,
		struct list_it   *fwd) 
{
	if (node->father)
		return;

	list_insert_one_at_tail(&node->ln, &to->sons, now, fwd);
	node->father = to;
}

/* detache a tree */
static __inline bool 
tree_detach(struct tree_node *node,
		struct list_it *now,
		struct list_it *fwd)
{
	bool res;
	if (NULL == node->father)
		return LIST_RET_BREAK;

	res = list_detach_one(&node->ln, &node->father->sons, now, fwd);
	node->father = NULL;

	return res;
}

/* this type is to save depth */
typedef unsigned short tree_depth_t;

/* 
 * the difference between tree_it_fun and list_it_fun is the
 * first add a depth argument.
 */
typedef bool tree_it_fun(struct list_it*, 
		struct list_it*, 
		struct list_it*,
		tree_depth_t,
		void*);

#define TREE_IT_CALLBK(_fun_name) \
	bool _fun_name(struct list_it *pa_head, \
			struct list_it *pa_now, \
			struct list_it *pa_fwd, \
			tree_depth_t pa_depth, \
			void *pa_extra)

/*
 * here are ways to traversal a tree (used as function callback)
 */
list_it_fun tree_post_order_DFS;
list_it_fun tree_pre_order_DFS;

/* 
 * use this function to go through a tree.
 * the second argument is the traversal methods listed before;
 * the fourth argument, if true, go through a tree ignoring 
 * the root;
 * the last argument is anything else you need as an extra 
 * argument.
 */
void
tree_foreach(struct tree_node *, list_it_fun *,
		tree_it_fun *, bool, void *);

#define TREE_OBJ(_type, _name, _tree_node_name) \
	_type* _name = MEMBER_2_STRUCT(pa_now->now, _type, _tree_node_name .ln)
