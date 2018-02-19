#include <stdlib.h>

#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"  /* for token_id */
#include "tex-parser/gen-symbol.h" /* for symbol_id */
#include "tex-parser/trans.h"      /* for trans_token() */
#include "head.h"

struct add_subpaths_args {
	uint32_t new_uniq;
	uint32_t new_dups;
	list    *set;
	uint32_t prefix_len;
	uint32_t keep_going;
};

static void _print_subpath(struct subpath *sp, uint32_t prefix_len);

struct test_interest_args {
	uint32_t interest;
	uint32_t cnt, max;
};

int interested_token(enum token_id tokid)
{
	enum token_id rank_base = T_MAX_RANK - OPTR_INDEX_RANK_MAX;
	if (rank_base < tokid && tokid < T_MAX_RANK) {
		//printf("Not interested at subr token %s.\n", trans_token(tokid));
		return 0;
	} else {
		//printf("subr token %s is interesting.\n", trans_token(tokid));
		return 1;
	}
}

static LIST_IT_CALLBK(test_interest)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct test_interest_args, pa_extra);

	arg->cnt ++;

	if (arg->cnt == arg->max) {
		arg->interest = interested_token(sp_nd->token_id);
		return LIST_RET_BREAK;
	} else if (pa_now->now == pa_head->last) {
		arg->interest = 1;
		return LIST_RET_BREAK;
	} else {
		return LIST_RET_CONTINUE;
	}
}

uint32_t
interesting_prefix(struct subpath *sp, uint32_t prefix_len)
{
	struct test_interest_args arg = {0, 0, prefix_len};
	list_foreach(&sp->path_nodes, &test_interest, &arg);
	return arg.interest;
}

static LIST_IT_CALLBK(add_subpaths)
{
	struct subpath_ele_added added;
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(args, struct add_subpaths_args, pa_extra);

	if (!interesting_prefix(sp, args->prefix_len)) {
		// if this level are not interesting tokens, we
		// still need to go next level in case there are
		// nodes there.
		args->keep_going = 1;
		LIST_GO_OVER;
	}

	added = subpath_set_add(args->set, sp, args->prefix_len);
	args->new_uniq += added.new_uniq;
	args->new_dups += added.new_dups;

#ifdef DEBUG_SUBPATH_SET
	fprintf(stdout, "after adding subpath: \n");
	_print_subpath(sp, args->prefix_len);
	printf("\n");
#endif
#ifdef DEBUG_SUBPATH_SET
	fprintf(stdout, "subpath set becomes: \n");
	subpath_set_print(args->set, stdout);
	printf("\n");
#endif

	LIST_GO_OVER;
}

struct subpath_ele_added
prefix_subpath_set_from_subpaths(struct subpaths* subpaths, list *set)
{
	struct subpath_ele_added ret = {0, 0};

	for (uint32_t l = 2;; l++) {
		struct add_subpaths_args args = {0, 0, set, l, 0};
		list_foreach(&subpaths->li, &add_subpaths, &args);
		ret.new_uniq += args.new_uniq;
		ret.new_dups += args.new_dups;

		if (args.new_dups == 0 && !args.keep_going)
			break;
	}

	return ret;
}

struct subpath_ele_added
lr_subpath_set_from_subpaths(struct subpaths* subpaths, list *set)
{
	struct subpath_ele_added ret = {0, 0};
	struct add_subpaths_args args = {0, 0, set, 0, 0};

	if (NULL == subpaths ||
	    subpaths->n_lr_paths > MAX_MATH_PATHS)
		return ret;

	list_foreach(&subpaths->li, &add_subpaths, &args);

	ret.new_uniq = args.new_uniq;
	ret.new_dups = args.new_dups;
	return ret;
}

static void _print_subpath(struct subpath *sp, uint32_t prefix_len)
{
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	math_index_mk_prefix_path_str(sp, prefix_len, path);
	printf("%s ", path);
}

static LIST_IT_CALLBK(print_subpath)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(fh, FILE, pa_extra);
	uint32_t i;

	_print_subpath(ele->dup[0], ele->prefix_len);
	printf(" prefix_len=%u ", ele->prefix_len);

	printf("(duplicates: ");
	for (i = 0; i <= ele->dup_cnt; i++)
		fprintf(fh, "%u~path#%u ", ele->rid[i], ele->dup[i]->path_id);
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
	int cnt_cmp_nodes;
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

		/* reaching max compare? */
		if (arg->max_cmp_nodes != 0) {
			arg->cnt_cmp_nodes ++;
			if (arg->cnt_cmp_nodes == arg->max_cmp_nodes) {
				arg->res = 0;
				return LIST_RET_BREAK;
			}
		}

		/* for tokens compare */
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

int sp_tokens_comparer(struct subpath *sp1, struct subpath *sp2, int prefix_len)
{
	struct _cmp_subpath_nodes_arg arg;
	arg.path_node2 = sp2->path_nodes;
	arg.path_node2_end = sp2->path_nodes.last;
	arg.res = 0;
	arg.skip_the_first = 0;
	arg.max_cmp_nodes = prefix_len;
	arg.cnt_cmp_nodes = 0;

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
	_print_subpath(sp1, prefix_len);
	printf(" and ");
	_print_subpath(sp2, prefix_len);
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

static struct subpath_ele *new_ele(struct subpath *sp, uint32_t prefix_len)
{
	struct subpath_ele *newele;
	newele = malloc(sizeof(struct subpath_ele));
	LIST_NODE_CONS(newele->ln);
	newele->dup_cnt = 0;
	newele->dup[0] = sp;
	newele->rid[0] = get_subpath_nodeid_at(sp, prefix_len);
	newele->prefix_len = prefix_len;

	return newele;
}

struct _subpath_set_add_args {
	struct subpath *subpath;
	uint32_t        prefix_len;
	uint32_t        new_uniq_inserted;
};

static LIST_IT_CALLBK(set_add)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(args, struct _subpath_set_add_args, pa_extra);
	struct subpath     *sp = args->subpath;
	struct subpath_ele *newele;

	if (ele->prefix_len == args->prefix_len &&
	    0 == sp_tokens_comparer(sp, ele->dup[0], args->prefix_len)) {
		ele->dup_cnt ++;
		ele->dup[ele->dup_cnt] = sp;
		ele->rid[ele->dup_cnt] = get_subpath_nodeid_at(sp, ele->prefix_len);
		return LIST_RET_BREAK;
	}

	if (pa_now->now == pa_head->last) {
		newele = new_ele(sp, args->prefix_len);
		list_insert_one_at_tail(&newele->ln, pa_head, pa_now, pa_fwd);

		/* indicate we inserted one into set */
		args->new_uniq_inserted = 1;

		return LIST_RET_BREAK;
	} else {
		/* not the last element */
		return LIST_RET_CONTINUE;
	}
}

struct subpath_ele_added
subpath_set_add(list *set, struct subpath *sp, int prefix_len)
{
	struct subpath_ele_added ret = {0, 0};
	struct subpath_ele *newele;

	if (prefix_len > sp->n_nodes)
		return ret;
	else if (prefix_len == 0)
		prefix_len = sp->n_nodes;
	/* if prefix_len is zero, indicating it is the full leaf-root path */

	if (set->now == NULL) {
		newele = new_ele(sp, prefix_len);
		list_insert_one_at_tail(&newele->ln, set, NULL, NULL);
		ret.new_uniq ++;
		ret.new_dups ++;
	} else {
		struct _subpath_set_add_args args = {sp, prefix_len, 0};
		list_foreach(set, &set_add, &args);

		if (args.new_uniq_inserted) {
			/* we just inserted an unique element */
			ret.new_uniq ++;
		}
		ret.new_dups ++;
	}

	return ret;
}

LIST_DEF_FREE_FUN(subpath_set_free, struct subpath_ele, ln, free(p));

static LIST_IT_CALLBK(dele_if_gener)
{
	bool res;
	LIST_OBJ(struct subpath, sp, ln);

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		res = list_detach_one(pa_now->now, pa_head, pa_now, pa_fwd);
		subpath_free(sp);
		return res;
	}

	LIST_GO_OVER;
}

void delete_gener_paths(struct subpaths *subpaths)
{
	list_foreach(&subpaths->li, &dele_if_gener, NULL);
}
