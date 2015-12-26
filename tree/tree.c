#include "tree.h"

/* some static structs */
struct traversal_arg {
	tree_it_fun *each_do;
	void        *extra;
};

struct foreach_arg {
	tree_depth_t          depth;
	struct traversal_arg *t_arg;
	bool                  only_once;
};

/* traversal methods implements */
LIST_IT_CALLBK(tree_post_order_DFS)
{
	LIST_OBJ(struct tree_node, node, ln);
	P_CAST(f_arg, struct foreach_arg, pa_extra);
	struct foreach_arg lo_f_arg = {f_arg->depth + 1, 
	                               f_arg->t_arg, 0};
	bool res;

	list_foreach(&node->sons, &tree_post_order_DFS, &lo_f_arg);

	res = (*f_arg->t_arg->each_do)(pa_head, pa_now, pa_fwd, 
	                      lo_f_arg.depth, f_arg->t_arg->extra);
	
	if (f_arg->only_once)
		return LIST_RET_BREAK;
	else
		return res;
}

LIST_IT_CALLBK(tree_pre_order_DFS)
{
	LIST_OBJ(struct tree_node, node, ln);
	P_CAST(f_arg, struct foreach_arg, pa_extra);
	struct foreach_arg lo_f_arg = {f_arg->depth + 1, 
	                               f_arg->t_arg, 0};
	bool res;

	res = (*f_arg->t_arg->each_do)(pa_head, pa_now, pa_fwd, 
                          lo_f_arg.depth, f_arg->t_arg->extra);

	list_foreach(&node->sons, &tree_pre_order_DFS, &lo_f_arg);

	if (f_arg->only_once)
		return LIST_RET_BREAK;
	else
		return res;
}

/* go through a tree, see tree.h for detail. */
void
tree_foreach(struct tree_node *root, list_it_fun *traversal, 
		tree_it_fun *each_do, bool if_exclude_root, void *extra)
{
	struct list_it tmp = list_get_it(&root->ln);
	struct traversal_arg t_arg = {each_do, extra};
	struct foreach_arg   f_arg = {0, &t_arg, 0};

	if (if_exclude_root) {
		f_arg.depth ++;
		list_foreach(&root->sons, traversal, &f_arg);
	} else {
		f_arg.only_once = 1;
		list_foreach(&tmp, traversal, &f_arg);
	}
}
