#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "math-expr-search.h"

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
	sort_arg.extra = NULL;
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
	uint32_t                    n_qry_lr_paths;
	struct postmerge           *pm;
	post_merge_callbk           post_on_merge;
	struct postmerge_callbks   *calls;
	uint32_t                    n_dir_visits;
	void                       *expr_srch_arg;
};

static enum dir_merge_ret
on_dir_merge(math_posting_t postings[MAX_MATH_PATHS], uint32_t n_postings,
             uint32_t level, void *args)
{
	P_CAST(on_dm_args, struct on_dir_merge_args, args);
	struct postmerge *pm = on_dm_args->pm;
	struct math_extra_score_arg mes_arg;
	bool res;
	uint32_t i;
	math_posting_t po;
	struct subpath_ele *ele;

	postmerge_posts_clear(pm);

	for (i = 0; i < n_postings; i++) {
		po = postings[i];
		ele = math_posting_get_ele(po);

#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("adding posting[%d]", i);
		math_posting_print_info(po);
		printf("\n");
#endif
		postmerge_posts_add(pm, po, on_dm_args->calls, ele);
	}

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("start merging math posting lists...\n");
#endif

	mes_arg.n_qry_lr_paths  = on_dm_args->n_qry_lr_paths;
	mes_arg.dir_merge_level = level;
	mes_arg.n_dir_visits    = on_dm_args->n_dir_visits;
	mes_arg.stop_dir_search = 0;
	mes_arg.expr_srch_arg   = on_dm_args->expr_srch_arg;

	res = posting_merge(pm, POSTMERGE_OP_AND,
	                    on_dm_args->post_on_merge, &mes_arg);

	/* increment directory visit counter */
	on_dm_args->n_dir_visits ++;

	if (!res || mes_arg.stop_dir_search) {
#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("math posting merge force-stopped.");
#endif
		return DIR_MERGE_RET_STOP;
	}

	return DIR_MERGE_RET_CONTINUE;
}

int math_expr_search(math_index_t mi, char *tex,
                     enum dir_merge_type dir_merge_type,
                     post_merge_callbk fun, void *args)
{
	struct tex_parse_ret     parse_ret;
	struct postmerge         pm;
	struct on_dir_merge_args on_dm_args;
	struct postmerge_callbks calls;

	calls.start = &math_posting_start;
	calls.finish = &math_posting_finish;
	calls.jump = &math_posting_jump;
	calls.next = &math_posting_next;
	calls.now = &math_posting_current_wrap;
	calls.now_id = &math_posting_current_id_wrap;

	/* parse TeX */
	parse_ret = tex_parse(tex, 0, false);

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
		printf("math query in order:\n");
		subpaths_print(&parse_ret.subpaths, stdout);
		printf("calling math_index_dir_merge()...\n");
#endif

		/* prepare directory merge extra arguments */
		on_dm_args.pm = &pm;
		on_dm_args.n_qry_lr_paths = parse_ret.subpaths.n_lr_paths;
		on_dm_args.post_on_merge = fun;
		on_dm_args.calls = &calls;
		on_dm_args.n_dir_visits = 0;
		on_dm_args.expr_srch_arg = args;

		math_index_dir_merge(mi, dir_merge_type, &parse_ret.subpaths,
		                     &on_dir_merge, &on_dm_args);

		subpaths_release(&parse_ret.subpaths);
	} else {
#ifdef DEBUG_MATH_EXPR_SEARCH
		printf("parser error: %s\n", parse_ret.msg);
#endif
		return 1;
	}

	return 0;
}

static __inline uint32_t
math_expr_sim(mnc_score_t mnc_score,
              uint32_t depth_delta, uint32_t breath_delta)
{
	uint32_t mult = (depth_delta + 1) * (breath_delta + 1);
	uint32_t score = (mult * mnc_score + 1) / mult;

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("mnc score = %u, depth delta = %u, breath delta = %u\n",
	       mnc_score, depth_delta, breath_delta);
#endif
	return score;
}

struct math_expr_score_res
math_expr_score_on_merge(struct postmerge* pm,
                         uint32_t level, uint32_t n_qry_lr_paths)
{
	uint32_t                    i, j, k;
	math_posting_t              posting;
	uint32_t                    pathinfo_pos;
	struct math_posting_item   *po_item;
	struct math_pathinfo_pack  *pathinfo_pack;
	struct math_pathinfo       *pathinfo;
	struct subpath_ele         *subpath_ele;
	bool                        skipped = 0;
	struct math_expr_score_res  ret;

	/* reset mnc for scoring new document */
	uint32_t slot;
	struct mnc_ref mnc_ref;
	mnc_reset_docs();

	for (i = 0; i < pm->n_postings; i++) {
		/* for each merged posting item from posting lists */
		posting = pm->postings[i];
		po_item = pm->cur_pos_item[i];
		subpath_ele = math_posting_get_ele(posting);
		assert(NULL != subpath_ele);

		/* get pathinfo position of corresponding merged item */
		pathinfo_pos = po_item->pathinfo_pos;

		/* use pathinfo position to get pathinfo packet */
		pathinfo_pack = math_posting_pathinfo(posting, pathinfo_pos);
		assert(NULL != pathinfo_pack);

		if (n_qry_lr_paths > pathinfo_pack->n_lr_paths) {
			/* impossible to match, skip this math expression */
#ifdef DEBUG_MATH_EXPR_SEARCH
			printf("query leaf-root paths (%u) is greater than "
			       "document leaf-root paths (%u), skip this expression."
			       "\n", n_qry_lr_paths, pathinfo_pack->n_lr_paths);
#endif
			skipped = 1;
			break;
		}

		for (j = 0; j < pathinfo_pack->n_paths; j++) {
			/* for each pathinfo from this pathinfo packet */
			pathinfo = pathinfo_pack->pathinfo + j;

			/* preparing to score corresponding document subpaths */
			mnc_ref.sym = pathinfo->lf_symb;
			slot = mnc_map_slot(mnc_ref);

			for (k = 0; k <= subpath_ele->dup_cnt; k++) {
				/* add this document subpath for scoring */
				mnc_doc_add_rele(slot, pathinfo->path_id,
				                 subpath_ele->dup[k]->path_id);
			}
		}
	}

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("query leaf-root paths: %u\n", n_qry_lr_paths);
	printf("document leaf-root paths: %u\n", pathinfo_pack->n_lr_paths);
	printf("posting merge level: %u\n", level);
#endif

	/* finally calculate expression similarity score */
	if (!skipped && pm->n_postings != 0) {
		ret.score = math_expr_sim(mnc_score(), level,
		                          pathinfo_pack->n_lr_paths - n_qry_lr_paths);
		ret.doc_id = po_item->doc_id;
		ret.exp_id = po_item->exp_id;
	} else {
		ret.score = 0;
	}

	return ret;
}
