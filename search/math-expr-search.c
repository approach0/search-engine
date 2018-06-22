#include <stdlib.h>
#include <stdio.h>

#include "tex-parser/head.h"
#include "postmerge/mergecalls.h"
#include "indices/indices.h"

#include "config.h"
#include "math-expr-search.h"
#include "mnc-score.h"

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

	/* assign path_id in order, from 1 to maximum 64. */
	sp->path_id = ++(*new_path_id);

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(push_query_path)
{
	uint32_t q_path_id;
	LIST_OBJ(struct subpath, sp, ln);
	struct mnc_ref mnc_ref;

	mnc_ref.sym = sp->lf_symbol_id;
	q_path_id = mnc_push_qry(mnc_ref);

	(void)q_path_id;
//	printf("MNC: push query path#%u %s\n", q_path_id,
//	       trans_symbol(mnc_ref.sym));

	LIST_GO_OVER;
}

static void prepare_score_struct(struct subpaths *subpaths)
{
	/* initialize 'mark and cross' query dimension */
	mnc_reset_qry();

	/* push queries to MNC stack for future scoring */
	list_foreach(&subpaths->li, &push_query_path, NULL);
}

static int prepare_math_qry(struct subpaths *subpaths)
{
	struct list_sort_arg sort_arg;
	uint32_t new_path_id = 0;

	/* strip gener paths because they are not used for searching */
	delete_gener_paths(subpaths);

	/* HACK: overwrite path_id of subpaths to the number of paths belong
	 * to this bond variable, i.e. bound variable size. */
	list_foreach(&subpaths->li, &overwrite_pathID_to_bondvar_sz, NULL);

	/* sort subpaths by <bound variable size, symbol> tuple */
	sort_arg.cmp = &compare_qry_path;
	sort_arg.extra = NULL;
	list_sort(&subpaths->li, &sort_arg);

	/* assign new path_id for each subpaths, in its list node order. */
	list_foreach(&subpaths->li, &assign_path_id_in_order, &new_path_id);

	/* prepare score structure for query subpaths */
	prepare_score_struct(subpaths);

	return 0;
}

struct on_dir_merge_args {
	struct indices             *indices;
	uint32_t                    n_qry_lr_paths;
	struct postmerge           *pm;
	struct subpath_ele        **eles;
	post_merge_callbk           post_on_merge;
	uint32_t                    n_dir_visits;
	void                       *expr_srch_arg;
	int64_t                     n_tot_rd_items;
	enum postmerge_op           posmerge_op;
	uint32_t                    n_max_qry_node_id;
};

enum dir_merge_ret
on_dir_merge(char (*full_paths)[MAX_MERGE_DIRS], char (*base_paths)[MAX_MERGE_DIRS],
	struct subpath_ele **eles, uint32_t n_postings, uint32_t level, void *args)
{
#if 0
	P_CAST(on_dm_args, struct on_dir_merge_args, args);
	struct postmerge *pm = on_dm_args->pm;
	struct math_extra_score_arg mes_arg;
	bool res;
	uint32_t i;
	math_posting_t po;
	struct subpath_ele *ele;

	postmerge_posts_clear(pm);

//		/* map a full path to math posting list */
//		postings[i] = math_posting_new_reader(dm_args->eles[i],
//		                               dm_args->full_paths[i]);

//	for (j = 0; j < i; j++)
//		math_posting_free_reader(postings[j]);

	for (i = 0; i < n_postings; i++) {
		po = postings[i];
		ele = math_posting_get_ele(po);

			sprintf(full_path, "../indexer/tmp/prefix/%s", q);
			printf("%s\n", full_path);
			if (math_posting_exits(full_path)) {
				prinfo("path not cached, get on-disk posting list.")
				po = math_posting_new_reader(indices.ti, full_path);
				po_arg = &main; /* indicate it is a math on-disk posting, free it later. */
				pm_calls = mergecalls_disk_math_postlist_v2();
			} else {
				prinfo("path not indexed, an empty posting list.")
				pm_calls = NULL_POSTMERGE_CALLS;
			}

#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("adding posting[%d]", i);
		math_posting_print_info(po);
		printf("\n");
#endif
		postmerge_posts_add(pm, po, , ele);
	}

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("start merging math posting lists...\n");
#endif

	mes_arg.n_qry_lr_paths  = on_dm_args->n_qry_lr_paths;
	mes_arg.dir_merge_level = level;
	mes_arg.n_dir_visits    = on_dm_args->n_dir_visits;
	mes_arg.stop_dir_search = 0;
	mes_arg.expr_srch_arg   = on_dm_args->expr_srch_arg;
	// printf("allocating prefix-query structure ...\n");
	mes_arg.pq = pq_allocate(on_dm_args->n_max_qry_node_id);

	res = posting_merge(pm, on_dm_args->posmerge_op,
	                    on_dm_args->post_on_merge, &mes_arg);

	pq_free(mes_arg.pq);

	/* increment total read item counter */
	on_dm_args->n_tot_rd_items += pm->n_rd_items;

	/* increment directory visit counter */
	on_dm_args->n_dir_visits ++;

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("rd, dir counter: %lu (limit: %u), %u (limit: %u)\n",
	       on_dm_args->n_tot_rd_items, MAX_MATH_EXP_SEARCH_ITEMS,
	       on_dm_args->n_dir_visits, MAX_MATH_EXP_SEARCH_DIRS);
#endif

	if (!res || mes_arg.stop_dir_search ||
	    on_dm_args->n_tot_rd_items > MAX_MATH_EXP_SEARCH_ITEMS ||
	    on_dm_args->n_dir_visits > MAX_MATH_EXP_SEARCH_DIRS) {

#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("math posting merge force-stopped.");
#endif
		return DIR_MERGE_RET_STOP;
	}

#endif
	return DIR_MERGE_RET_CONTINUE;
}

int64_t math_expr_search(struct indices *indices, char *tex,
                         enum math_expr_search_policy search_policy,
                         post_merge_callbk fun, void *args)
{
	struct tex_parse_ret     parse_ret;
	struct postmerge         pm;
	struct on_dir_merge_args on_dm_args;
	struct postmerge_callbks calls;

	enum dir_merge_type dir_merge_type = DIR_MERGE_DIRECT;
	enum dir_merge_pathset_type path_type = DIR_PATHSET_LEAFROOT_PATH;

	/* parse TeX */
	parse_ret = tex_parse(tex, 0, true);

	if (parse_ret.operator_tree) {
		on_dm_args.n_max_qry_node_id = optr_max_node_id((struct optr_node *)
		                                                parse_ret.operator_tree);
		optr_release((struct optr_node*)parse_ret.operator_tree);
#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("max query node id: %u\n", on_dm_args.n_max_qry_node_id);
#endif
	}

	if (parse_ret.code != PARSER_RETCODE_ERR) {
#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("before prepare_math_qry():\n");
		subpaths_print(&parse_ret.subpaths, stdout);
#endif
		/* prepare math query */
		prepare_math_qry(&parse_ret.subpaths);

#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("after prepare_math_qry():\n");
		subpaths_print(&parse_ret.subpaths, stdout);
#endif

#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("calling math_index_dir_merge()...\n");
#endif

		/* prepare directory merge extra arguments */
		on_dm_args.indices = indices;
		on_dm_args.pm = &pm;
		on_dm_args.n_qry_lr_paths = parse_ret.subpaths.n_lr_paths;
		on_dm_args.post_on_merge = fun;
		on_dm_args.n_dir_visits = 0;
		on_dm_args.expr_srch_arg = args;
		on_dm_args.n_tot_rd_items = 0;

		switch (search_policy) {
		case MATH_SRCH_FUZZY_STRUCT:
			on_dm_args.posmerge_op = POSTMERGE_OP_OR;
			dir_merge_type         = DIR_MERGE_DIRECT;
			path_type              = DIR_PATHSET_PREFIX_PATH;
			break;
		case MATH_SRCH_EXACT_STRUCT:
			on_dm_args.posmerge_op = POSTMERGE_OP_AND;
			dir_merge_type         = DIR_MERGE_DIRECT;
			path_type              = DIR_PATHSET_LEAFROOT_PATH;
			break;
		case MATH_SRCH_SUBEXPRESSION:
			on_dm_args.posmerge_op = POSTMERGE_OP_AND;
			dir_merge_type         = DIR_MERGE_BREADTH_FIRST;
			path_type              = DIR_PATHSET_LEAFROOT_PATH;
			break;
		default:
			fprintf(stderr, "Unknown search policy: %u\n", search_policy);
		}

		math_index_dir_merge(indices->mi, dir_merge_type, path_type,
		                     &parse_ret.subpaths, &on_dir_merge, &on_dm_args);

		subpaths_release(&parse_ret.subpaths);

		return on_dm_args.n_tot_rd_items;
	} else {
#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("parser error: %s\n", parse_ret.msg);
#endif
		return -1;
	}
}
