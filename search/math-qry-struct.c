#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* for subpath_set_free() */
#include "math-index/math-index.h"
#include "math-index/subpath-set.h"

#include "tex-parser/head.h"

#include "config.h"
#include "math-qry-struct.h"

static LIST_CMP_CALLBK(compare_qry_path)
{
	struct subpath *sp0 = MEMBER_2_STRUCT(pa_node0, struct subpath, ln);
	struct subpath *sp1 = MEMBER_2_STRUCT(pa_node1, struct subpath, ln);

	/* larger size bound variables are ranked higher, if sizes are equal,
	 * rank by path type (wildcard or concrete) then symbol (alphabet).*/
	if (sp0->pseudo != sp1->pseudo ||
	         sp0->type != sp1->type) {

		/* Order this case: normal (00), normal pseudo (01), wildcards (11) */
		if (sp0->type != sp1->type)
			return (sp0->type == SUBPATH_TYPE_NORMAL) ? 1 : 0;
		else
			return (sp0->pseudo) ? 0 : 1;

	}

	if (sp0->path_id != sp1->path_id)
		return sp0->path_id > sp1->path_id;

	return sp0->lf_symbol_id < sp1->lf_symbol_id;
}

struct cnt_same_symbol_args {
	uint32_t          cnt;
	symbol_id_t       symbol_id;
	enum subpath_type path_type;
	bool              pseudo;
};

static LIST_IT_CALLBK(cnt_same_symbol)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(cnt_arg, struct cnt_same_symbol_args, pa_extra);

	if (cnt_arg->symbol_id == sp->lf_symbol_id &&
	    cnt_arg->path_type == sp->type &&
	    cnt_arg->pseudo == sp->pseudo)
		cnt_arg->cnt ++;

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(overwrite_pathID_to_bondvar_sz)
{
	struct list_it this_list;
	struct cnt_same_symbol_args cnt_arg;
	LIST_OBJ(struct subpath, sp, ln);

	/* get iterator of this list */
	this_list = list_get_it(pa_head->now);

	/* go through this list to count subpaths with same symbol */
	cnt_arg.cnt = 0;
	cnt_arg.symbol_id = sp->lf_symbol_id;
	cnt_arg.path_type = sp->type;
	cnt_arg.pseudo    = sp->pseudo;
	list_foreach(&this_list, &cnt_same_symbol, &cnt_arg);

	/* overwrite path_id to cnt number */
	sp->path_id = cnt_arg.cnt;

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(assign_path_id_in_order)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(new_path_id, uint32_t, pa_extra);

	/* assign path_id in order, from 1 to maximum 64. */
	sp->path_id = ++(*new_path_id);

	LIST_GO_OVER;
}

struct find_conjugacy_args {
	struct subpath *sp;
	uint64_t conjugacy;
};

static LIST_IT_CALLBK(find_conjugacy)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct find_conjugacy_args, pa_extra);

	if (arg->sp != sp && arg->sp->leaf_id == sp->leaf_id) {
		arg->conjugacy |= (1L << (sp->path_id - 1));
	}

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(assign_conjugacy)
{
	struct list_it this_list;
	LIST_OBJ(struct subpath, sp, ln);

	/* get iterator of this list */
	this_list = list_get_it(pa_head->now);

	/* go through this list to find conjugacy */
	struct find_conjugacy_args arg = {sp, 0};
	list_foreach(&this_list, &find_conjugacy, &arg);

	/* assign conjugacy */
	sp->conjugacy = arg.conjugacy;

	LIST_GO_OVER;
}

static void
expand_as(struct subpaths *subpaths, struct optr_node *p,
          enum subpath_type type, enum token_id token)
{
	struct subpath *subpath;
	/* create a mirror normal path for wildcard path */
	subpath = create_subpath(p, true);
	subpath->type = type;
	/* generate nodes of this subpath */
	insert_subpath_nodes(subpath, p, token);
	/* generate fingerprint of this subpath */
	subpath->fingerprint = subpath_fingerprint(subpath, UINT32_MAX);
	/* insert this new subpath to subpath list */
	list_insert_one_at_tail(&subpath->ln, &subpaths->li, NULL, NULL);
	/* count total subpaths generated. */
	subpaths->n_subpaths ++;
}

static TREE_IT_CALLBK(_expand_path)
{
	P_CAST(subpaths, struct subpaths, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);

	/* reached the limit of maximum paths we can generate */
	if (subpaths->n_subpaths + 1 >= MAX_SUBPATHS)
		return LIST_RET_BREAK;

	if (p->tnd.sons.now == NULL /* is leaf */) {

		/* mirror only wildcards path, no need to increment number
		 * of leaf-root paths in subpaths. */
		if (p->wildcard) {
			expand_as(subpaths, p, SUBPATH_TYPE_NORMAL, T_VAR);
			expand_as(subpaths, p, SUBPATH_TYPE_NORMAL, T_NUM);
		}
	}

	LIST_GO_OVER;
}

static void
expand_query_subpaths(struct subpaths *subpaths, struct optr_node* optr)
{
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &_expand_path,
	             1 /* excluding root */, subpaths);
}

static int math_qry_print_visibi_map(uint32_t*);
static int math_qry_gen_visibi_map(uint32_t*, struct optr_node*);

int math_qry_prepare(struct indices *indices, char *tex, struct math_qry_struct* s)
{
	struct tex_parse_ret parse_ret;
	/* no gener paths since they are not used for searching */
	s->kw_str = strdup(tex);
	parse_ret = tex_parse(tex, 0, true, true);

	s->pq.n = 0;
	s->subpaths.n_lr_paths = 0;
	LIST_CONS(s->subpath_set);
	s->n_uniq_paths = 0;

	if (parse_ret.code == PARSER_RETCODE_ERR ||
	    parse_ret.operator_tree == NULL) {
		s->optr = NULL;
		return 1;
	}
	/* save OPT */
	s->optr = parse_ret.operator_tree;
#ifdef DEBUG_PRINT_QRY_STRUCT
	optr_print(s->optr, stdout);
#endif

	/* copy subpaths reference */
	struct subpaths subpaths = parse_ret.subpaths;
	/* clear the old reference (ensure no one uses that) */
	parse_ret.subpaths.li = list_get_it(NULL);

#ifdef WILDCARD_PATH_QUERY_EXPAND_ENABLE
	/* "mirror" wildcard path to also match single-symbol for better recall */
	expand_query_subpaths(&subpaths, parse_ret.operator_tree);
#endif

	/* generate query node visibility map */
	math_qry_gen_visibi_map(s->visibimap, parse_ret.operator_tree);
	
	/* sort subpaths by <bound variable size, path type, symbol> */
	list_foreach(&subpaths.li, &overwrite_pathID_to_bondvar_sz, NULL);
	struct list_sort_arg sort_arg = {&compare_qry_path, NULL};
	list_sort(&subpaths.li, &sort_arg);

	/* assign path_id in this new order. */
	uint32_t new_path_id = 0;
	list_foreach(&subpaths.li, &assign_path_id_in_order, &new_path_id);

	/* assign conjugacy bitmap */
	list_foreach(&subpaths.li, &assign_conjugacy, NULL);

#ifdef DEBUG_PRINT_QRY_STRUCT
	printf("Leaf-root paths (path_id sorted by bond-vars):\n");
	subpaths_print(&subpaths, stdout);
#endif

	/* prepare structure scoring structure */
	s->n_qry_nodes = optr_max_node_id(parse_ret.operator_tree);
	s->pq = pq_allocate(s->n_qry_nodes);
	
	/* create subpath set (unique paths for merging) */
	s->subpath_set = dir_merge_subpath_set(
		DIR_PATHSET_PREFIX_PATH, &subpaths, &s->n_uniq_paths
	);
#ifdef DEBUG_PRINT_QRY_STRUCT
	printf("Search path set: (%u unique paths)\n", s->n_uniq_paths);
	subpath_set_print(&s->subpath_set, stdout);
#endif

	/* set return subpaths */
	s->subpaths = subpaths;

	return 0;
}

void math_qry_free(struct math_qry_struct* s)
{
	if (s->optr) {
		optr_release((struct optr_node*)s->optr);
		s->optr = NULL;
	}

	if (s->subpath_set.now) {
		// subpath_set_print(&s->subpath_set, stdout);
		subpath_set_free(&s->subpath_set);
	}

	if (s->subpaths.n_lr_paths) {
		subpaths_release(&s->subpaths);
	}

	if (s->pq.n) {
		pq_free(s->pq);
	}

	if (s->kw_str) {
		free(s->kw_str);
	}
}

static int OPT_node_visible(enum token_id token_id)
{
	if (token_id <= T_MAX_RANK)
		return 0;

	switch (token_id) {
	case T_HANGER:
	case T_BASE:
	case T_SUPSCRIPT:
	case T_SUBSCRIPT:
	case T_PRE_SUPSCRIPT:
	case T_PRE_SUBSCRIPT:
//	case T_FRAC:
//	case T_TIMES:
		return 0;
	default:
		return 1;
	}
}

static TREE_IT_CALLBK(gen_visibi_map)
{
	P_CAST(map, uint32_t, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);

	assert(p->node_id < MAX_NODE_IDS);

	if (p->tnd.sons.now != NULL /* is not leaf */) {
		if (OPT_node_visible(p->token_id))
			map[p->node_id] = 1;
	}

	LIST_GO_OVER;
}

static int math_qry_gen_visibi_map(uint32_t *map, struct optr_node *optr)
{
	/* clear bitmap */
	memset(map, 0, sizeof(uint32_t) * MAX_NODE_IDS);

	tree_foreach(&optr->tnd, &tree_post_order_DFS, &gen_visibi_map,
	             0 /* including root */, map);
	return 0;
}

static int math_qry_print_visibi_map(uint32_t* map)
{
	printf("node#");
	for (int i = 0; i < MAX_SUBPATH_ID; i++) {
		if (map[i]) printf("%u ", i);
	}
	printf("\n");

	return 0;
}
