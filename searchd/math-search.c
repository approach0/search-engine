#include <stdlib.h>
#include <stdio.h>

#include "math-search.h"

static LIST_CMP_CALLBK(compare_qry_path)
{
	struct subpath *sp0 = MEMBER_2_STRUCT(pa_node0, struct subpath, ln);
	struct subpath *sp1 = MEMBER_2_STRUCT(pa_node1, struct subpath, ln);

	/* larger size bound variables are ranked higher, if sizes are equal,
	 * rank by symbol ID (kind of alphabet order). */
	if (sp0->path_id == sp1->path_id)
		return sp0->lf_symbol_id < sp1->lf_symbol_id;
	else
		return sp0->path_id > sp1->path_id;
}

static LIST_IT_CALLBK(dele_if_gener)
{
	bool res;
	LIST_OBJ(struct subpath, sp, ln);

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		res = list_detach_one(pa_now->now, pa_head, pa_now, pa_fwd);
		free(sp);
		return res;
	}

	LIST_GO_OVER;
}

struct cnt_same_symbol_args {
	uint32_t    cnt;
	symbol_id_t symbol_id;
};

static LIST_IT_CALLBK(cnt_same_symbol)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(cnt_arg, struct cnt_same_symbol_args, pa_extra);

	if (cnt_arg->symbol_id == sp->lf_symbol_id)
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
	list_foreach(&this_list, &cnt_same_symbol, &cnt_arg);

	/* overwrite path_id to cnt number */
	sp->path_id = cnt_arg.cnt;

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(assign_path_id_in_order)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(new_path_id, uint32_t, pa_extra);

	/* assign path_id in order */
	sp->path_id = (*new_path_id) ++;

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(push_query_path)
{
	LIST_OBJ(struct subpath, sp, ln);
	struct mnc_ref mnc_ref;

	mnc_ref.sym = sp->lf_symbol_id;
	mnc_push_qry(mnc_ref);

	LIST_GO_OVER;
}

static void prepare_score_struct(struct subpaths *subpaths)
{
	/* initialize 'mark and score' query dimension */
	mnc_reset_qry();

	/* push queries to MNC stack for future scoring */
	list_foreach(&subpaths->li, &push_query_path, NULL);
}

static int prepare_math_qry(struct subpaths *subpaths)
{
	struct list_sort_arg sort_arg;
	uint32_t new_path_id = 0;

	/* strip gener paths because they are not used for searching */
	list_foreach(&subpaths->li, &dele_if_gener, NULL);

	/* HACK: overwrite path_id of subpaths to the number of paths belong
	 * to this bond variable, i.e. bound variable size. */
	list_foreach(&subpaths->li, &overwrite_pathID_to_bondvar_sz, NULL);

	/* sort subpaths by <bound variable size, symbol> tuple */
	sort_arg.cmp = &compare_qry_path;
	list_sort(&subpaths->li, &sort_arg);

	/* assign new path_id for each subpaths, in its list node order. */
	list_foreach(&subpaths->li, &assign_path_id_in_order, &new_path_id);

	/* prepare score structure for query subpaths */
	prepare_score_struct(subpaths);

	return 0;
}

static void* math_posting_current_wrap(math_posting_t po_)
{
	return (void*)math_posting_current(po_);
}

static uint64_t math_posting_current_id_wrap(void *po_item_)
{
	/* this casting requires `struct math_posting_item' has
	 * docID and expID as first two structure members. */
	uint64_t *id64 = (uint64_t *)po_item_;

	return *id64;
};

struct on_dir_merge_args {
	uint32_t              n_qry_lr_paths;
	struct postmerge_arg *pm_arg;
};

static enum dir_merge_ret
on_dir_merge(math_posting_t postings[MAX_MATH_PATHS], uint32_t n_postings,
             uint32_t level, void *args)
{
	P_CAST(on_dm_args, struct on_dir_merge_args, args);
	struct postmerge_arg *pm_arg = on_dm_args->pm_arg;

	uint32_t i;
	math_posting_t po;
	struct subpath_ele *ele;

	postmerge_posts_clear(pm_arg);

	for (i = 0; i < n_postings; i++) {
		po = postings[i];
		ele = math_posting_get_ele(po);

#ifdef DEBUG_MATH_SEARCH
		printf("adding posting[%d]", i);
		math_posting_print_info(po);
		printf("\n");
#endif
		postmerge_posts_add(pm_arg, po, ele);
	}

#ifdef DEBUG_MATH_SEARCH
	printf("start merging math posting lists...\n");
#endif

	if (!posting_merge(pm_arg, &on_dm_args->n_qry_lr_paths)) {
#ifdef DEBUG_MATH_SEARCH
		fprintf(stderr, "math posting merge failed.");
#endif
		return DIR_MERGE_RET_STOP;
	}

	return DIR_MERGE_RET_CONTINUE;
}

int math_search_posting_merge(math_index_t mi, char *tex,
                              enum dir_merge_type dir_merge_type,
                              post_merge_callbk fun, void *args)
{
	struct tex_parse_ret parse_ret;
	struct postmerge_arg pm_arg;
	struct on_dir_merge_args on_dm_args;

	/*
	 * prepare term posting list merge callbacks.
	 */
	pm_arg.post_start_fun = &math_posting_start;
	pm_arg.post_finish_fun = &math_posting_finish;
	pm_arg.post_jump_fun = &math_posting_jump;
	pm_arg.post_next_fun = &math_posting_next;
	pm_arg.post_now_fun = &math_posting_current_wrap;
	pm_arg.post_now_id_fun = &math_posting_current_id_wrap;
	pm_arg.post_on_merge = fun;

	/* subpaths are always using AND merge */
	pm_arg.op = POSTMERGE_OP_AND;

	/* parse TeX */
	parse_ret = tex_parse(tex, 0, false);

	if (parse_ret.code == PARSER_RETCODE_SUCC) {
#ifdef DEBUG_MATH_SEARCH
		printf("before prepare_math_qry():\n");
		subpaths_print(&parse_ret.subpaths, stdout);
#endif
		/* prepare math query */
		prepare_math_qry(&parse_ret.subpaths);

#ifdef DEBUG_MATH_SEARCH
		printf("after prepare_math_qry():\n");
		subpaths_print(&parse_ret.subpaths, stdout);
#endif

#ifdef DEBUG_MATH_SEARCH
		printf("math query in order:\n");
		subpaths_print(&parse_ret.subpaths, stdout);
		printf("calling math_index_dir_merge()...\n");
#endif

		/* prepare directory merge extra arguments */
		on_dm_args.pm_arg = &pm_arg;
		on_dm_args.n_qry_lr_paths = parse_ret.subpaths.n_lr_paths;

		math_index_dir_merge(mi, dir_merge_type, &parse_ret.subpaths,
		                     &on_dir_merge, &on_dm_args);

		subpaths_release(&parse_ret.subpaths);
	} else {
#ifdef DEBUG_MATH_SEARCH
		printf("parser error: %s\n", parse_ret.msg);
#endif
		return 1;
	}

	return 0;
}
