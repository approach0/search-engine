#include <stdlib.h>

#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"  /* for token_id */
#include "tex-parser/gen-symbol.h" /* for symbol_id */
#include "tex-parser/trans.h"      /* for trans_token() */
#include "head.h"

struct add_subpaths_args {
	uint32_t              n_uniq;
	list                 *set;
	subpath_set_comparer *cmp;
};

static LIST_IT_CALLBK(add_subpaths)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(args, struct add_subpaths_args, pa_extra);

	if (0 == subpath_set_add(args->set, sp, args->cmp))
		args->n_uniq ++;

	LIST_GO_OVER;
}

uint32_t
subpath_set_from_subpaths(struct subpaths* subpaths,
                          subpath_set_comparer *cmp, list *set)
{
	struct add_subpaths_args args = {0, set, cmp};

	if (NULL == subpaths ||
	    subpaths->n_lr_paths > MAX_MATH_PATHS)
		return 0;

	list_foreach(&subpaths->li, &add_subpaths, &args);

	return args.n_uniq;
}

static void _print_subpath(struct subpath *sp)
{
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	math_index_mk_path_str(sp, path);
	printf("%s ", path);
}

static LIST_IT_CALLBK(print_subpath)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(fh, FILE, pa_extra);
	uint32_t i;

	_print_subpath(ele->dup[0]);

	printf("(duplicates: ");
	for (i = 0; i <= ele->dup_cnt; i++)
		fprintf(fh, "path#%u ", ele->dup[i]->path_id);
	printf(")\n");

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
	int skip_the_first;
	int max_cmp_nodes;
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

	if (n1->token_id == n2->token_id || arg->skip_the_first) {
		arg->skip_the_first = 0;

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

int sp_tokens_comparer(struct subpath *sp1, struct subpath *sp2)
{
	struct _cmp_subpath_nodes_arg arg;
	arg.path_node2 = sp2->path_nodes;
	arg.path_node2_end = sp2->path_nodes.last;
	arg.res = 0;
	arg.skip_the_first = 0;

	if (sp1->type != sp2->type) {
		arg.res = 5;
	} else {
		if (sp1->type == SUBPATH_TYPE_GENERNODE)
			arg.skip_the_first = 1;
		else if (sp1->type == SUBPATH_TYPE_WILDCARD)
			arg.skip_the_first = 1;

		list_foreach(&sp1->path_nodes, &cmp_subpath_nodes, &arg);
	}

//#define DEBUG_SUBPATH_SET
#ifdef DEBUG_SUBPATH_SET
	_print_subpath(sp1);
	printf(" and ");
	_print_subpath(sp2);
	printf(" ");

	switch (arg.res) {
	case 0:
		printf("are the same.");
		break;
	case 1:
		printf("are different. (the other is empty)");
		break;
	case 2:
		printf("are different. (the other is shorter)");
		break;
	case 3:
		printf("are different. (the other is longer)");
		break;
	case 4:
		printf("are different. (node tokens do not match)");
		break;
	case 5:
		printf("are different. (the other is not the same type)");
		break;
	default:
		printf("unexpected res number.\n");
	}
	printf("\n");
#endif

	return arg.res;
}

int sp_prefix_comparer(struct subpath *sp1, struct subpath *sp2)
{
	struct _cmp_subpath_nodes_arg arg;
	arg.path_node2 = sp2->path_nodes;
	arg.path_node2_end = sp2->path_nodes.last;
	arg.res = 0;
	arg.skip_the_first = 0;

	list_foreach(&sp1->path_nodes, &cmp_subpath_nodes, &arg);
}

static struct subpath_ele *new_ele(struct subpath *sp)
{
	struct subpath_ele *newele;
	newele = malloc(sizeof(struct subpath_ele));
	LIST_NODE_CONS(newele->ln);
	newele->dup_cnt = 0;
	newele->dup[0] = sp;

	return newele;
}

struct _subpath_set_add_args {
	struct subpath       *subpath;
	subpath_set_comparer *cmp;
};

static LIST_IT_CALLBK(set_add)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(args, struct _subpath_set_add_args, pa_extra);
	struct subpath     *sp = args->subpath;
	struct subpath_ele *newele;

	if (0 == (*args->cmp)(sp, ele->dup[0])) {
		ele->dup_cnt ++;
		ele->dup[ele->dup_cnt] = sp;
		return LIST_RET_BREAK;
	} else {
		if (pa_now->now == pa_head->last) {
			newele = new_ele(sp);
			list_insert_one_at_tail(&newele->ln, pa_head,
			                        pa_now, pa_fwd);

			/* indicate we inserted one into set */
			pa_now->now = NULL;

			return LIST_RET_BREAK;
		} else {
			/* not the last element */
			return LIST_RET_CONTINUE;
		}
	}
}

bool
subpath_set_add(list *set, struct subpath *sp, subpath_set_comparer* cmp)
{
	struct subpath_ele *newele;
	struct list_it br;

	if (set->now == NULL) {
		newele = new_ele(sp);
		list_insert_one_at_tail(&newele->ln, set, NULL, NULL);
		return 0;
	} else {
		struct _subpath_set_add_args args = {sp, cmp};
		br = list_foreach(set, &set_add, &args);

		if (br.now == NULL) {
			/* we just inserted an unique element */
			return 0;
		}
	}

	return 1;
}

LIST_DEF_FREE_FUN(subpath_set_free, struct subpath_ele, ln, free(p));
