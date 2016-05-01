#include <stdlib.h>

#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"  /* for token_id */
#include "tex-parser/gen-symbol.h" /* for symbol_id */
#include "tex-parser/trans.h"      /* for trans_token() */

#include "math-index.h"
#include "subpath-set.h"
#include "config.h"

static LIST_IT_CALLBK(print_subpath_nodes)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(fh, FILE, pa_extra);

	fprintf(fh, "%s/", trans_token(sp_nd->token_id));
	LIST_GO_OVER;
}

static LIST_IT_CALLBK(print_subpath)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(fh, FILE, pa_extra);

	list_foreach(&ele->path_nodes, &print_subpath_nodes, fh);
	fprintf(fh, " (dup_cnt: %u)\n", ele->dup_cnt);
	LIST_GO_OVER;
}

void subpath_set_print(list *set, FILE *fh)
{
	list_foreach(set, &print_subpath, fh);
}

struct _cmp_subpath_nodes_arg {
	struct list_it    path_node2;
	struct list_node *path_node2_end;
	int res;
};

static LIST_IT_CALLBK(cmp_subpath_nodes)
{
	LIST_OBJ(struct subpath_node, n1, ln);
	P_CAST(arg, struct _cmp_subpath_nodes_arg, pa_extra);
	struct subpath_node *n2 = MEMBER_2_STRUCT(arg->path_node2.now,
	                                          struct subpath_node, ln);

	if (n2 == NULL) {
		arg->res = 1;
		return LIST_RET_BREAK;
	}

	if (n1->token_id == n2->token_id) {
		if (arg->path_node2.now == arg->path_node2_end) {
			if (pa_now->now == pa_head->last)
				arg->res = 0;
			else
				arg->res = 2;

			return LIST_RET_BREAK;
		} else {
			if (pa_now->now == pa_head->last) {
				arg->res = 3;
				return LIST_RET_BREAK;
			} else {
				arg->path_node2 = list_get_it(arg->path_node2.now->next);
				return LIST_RET_CONTINUE;
			}
		}
	} else {
		arg->res = 4;
		return LIST_RET_BREAK;
	}
}

static int compare(struct subpath_ele *ele1, struct subpath_ele *ele2)
{
	struct _cmp_subpath_nodes_arg arg;
	arg.path_node2 = ele2->path_nodes;
	arg.path_node2_end = ele2->path_nodes.last;
	arg.res = 0;

	if (ele1->subpath_type != ele2->subpath_type)
		arg.res = 5;
	else
		list_foreach(&ele1->path_nodes, &cmp_subpath_nodes, &arg);

#ifdef DEBUG_SUBPATH_SET
	list_foreach(&ele1->path_nodes, &print_subpath_nodes, stdout);
	printf(" and ");
	list_foreach(&ele2->path_nodes, &print_subpath_nodes, stdout);
	printf(" ");
	if (arg.res == 0)
		printf("are the same.");
	else if (arg.res == 1)
		printf("are different. (the other is empty)");
	else if (arg.res == 2)
		printf("are different. (the other is short)");
	else if (arg.res == 3)
		printf("are different. (the other is long)");
	else if (arg.res == 4)
		printf("are different.");
	else if (arg.res == 5)
		printf("are different. (the other is not the same type)");
	printf("\n");
#endif

	return arg.res;
}

static LIST_IT_CALLBK(set_add)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(newele, struct subpath_ele, pa_extra);

	if (0 == compare(newele, ele)) {
		ele->dup_cnt ++;
		free(newele);
		return LIST_RET_BREAK;
	} else {
		if (pa_now->now == pa_head->last) {
			list_insert_one_at_tail(&newele->ln, pa_head, pa_now, pa_fwd);
			return LIST_RET_BREAK;
		} else {
			return LIST_RET_CONTINUE;
		}
	}
}

void subpath_set_add(list *set, struct subpath *sp,
                     struct math_posting_item *to_write)
{
	struct subpath_ele *newele = malloc(sizeof(struct subpath_ele));
	LIST_NODE_CONS(newele->ln);
	newele->path_nodes = sp->path_nodes;
	newele->to_write = *to_write;
	newele->subpath_type = sp->type;
	newele->dup_cnt = 0;

	if (set->now == NULL)
		list_insert_one_at_tail(&newele->ln, set, NULL, NULL);
	else
		list_foreach(set, &set_add, newele);
}

LIST_DEF_FREE_FUN(subpath_set_free, struct subpath_ele, ln, free(p));
