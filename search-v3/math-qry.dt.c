#include <stdio.h>
#include "tex-parser/head.h"
#include "config.h"
#include "math-qry.h"
#include "math-score.h" /* for IPF calculation */

/*
 * functions to sort subpaths by bound variable size
 */
struct cnt_same_symbol_args {
	uint32_t          cnt;
	symbol_id_t       symbol_id;
	enum subpath_type path_type;
	bool              pseudo;
};

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

static LIST_IT_CALLBK(assign_pathID_by_order)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(new_path_id, uint32_t, pa_extra);

	/* assign path_id in order, from 1 to maximum 64. */
	sp->path_id = ++(*new_path_id);

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(add_mnc_qry_path)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(mq, struct math_qry, pa_extra);

	mnc_score_qry_path_add(&mq->mnc, sp->lf_symbol_id);

	LIST_GO_OVER;
}

/*
 * main exported functions
 */
int math_qry_prepare(math_index_t mi, const char *tex, struct math_qry *mq)
{
	memset(mq, 0, sizeof *mq);

	/*
	 * parse TeX
	 */
	struct tex_parse_ret parse_res;
	parse_res = tex_parse(tex, 0, true /* keep OPT */, true /* concrete */);
	if (parse_res.code == PARSER_RETCODE_ERR ||
	    parse_res.operator_tree == NULL) {
		return 1;
	}

	/*
	 * save TeX
	 */
	mq->tex = strdup(tex);
#ifdef DEBUG_PREPARE_MATH_QRY
	printf("%s\n", tex);
#endif

	/*
	 * save OPT
	 */
	mq->optr = parse_res.operator_tree;
#ifdef DEBUG_PREPARE_MATH_QRY
	optr_print(mq->optr, stdout);
#endif

	/*
	 * calculate the number of query nodes
	 */
	mq->n_qnodes = optr_max_node_id(mq->optr);

	/*
	 * prepare query path mnc structure
	 */
	struct subpaths subpaths = parse_res.subpaths;
	mnc_score_init(&mq->mnc);
	list_foreach(&subpaths.li, &add_mnc_qry_path, mq);
	mnc_score_qry_path_sort(&mq->mnc);
#ifdef DEBUG_PREPARE_MATH_QRY
	mnc_score_print(&mq->mnc, 0);
#endif

	/*
	 * save subpaths and sort them by bond variable size
	 */
	list_foreach(&subpaths.li, &overwrite_pathID_to_bondvar_sz, NULL);

	struct list_sort_arg sort_arg = {&compare_qry_path, NULL};
	list_sort(&subpaths.li, &sort_arg);

	uint32_t new_path_id = 0;
	list_foreach(&subpaths.li, &assign_pathID_by_order, &new_path_id);

	mq->subpaths = subpaths;

#ifdef DEBUG_PREPARE_MATH_QRY
	subpaths_print(&subpaths, stdout);
#endif

	/*
	 * save subpath set
	 */
	mq->subpath_set = subpath_set(subpaths, SUBPATH_SET_QUERY);

#ifdef DEBUG_PREPARE_MATH_QRY
	print_subpath_set(mq->subpath_set);
	printf("\n");
#endif

	/*
	 * setup merger for elements in subpath set
	 */
	float N  = mi->stats.N;

	foreach (iter, li, mq->subpath_set) {
		struct subpath_ele *ele = li_entry(ele, iter->cur, ln);
		struct subpath *sp = ele->dup[0];
		char path_key[MAX_DIR_PATH_NAME_LEN] = "";

		if (0 != mk_path_str(sp, ele->prefix_len, path_key)) {
			prerr("subpath too long or unexpected type.\n");
			continue;
		}

		unsigned int n = mq->merge_set.n;
		mq->entry[n] = math_index_lookup(mi, path_key);

		if (mq->entry[n].pf) {
			mq->ipf[n] = math_score_ipf(N, mq->entry[n].pf);
			mq->merge_set.iter[n] = mq->entry[n].reader;
			mq->merge_set.upp [n] = mq->ipf[n]; /* here is single path IPF */
			mq->merge_set.cur [n] = (merger_callbk_cur)invlist_iter_curkey;
			mq->merge_set.next[n] = (merger_callbk_next)invlist_iter_next;
			mq->merge_set.skip[n] = (merger_callbk_skip)invlist_iter_jump;
			mq->merge_set.read[n] = (merger_callbk_read)invlist_iter_read;
		} else {
			mq->ipf[n] = 0;
			mq->merge_set.iter[n] = NULL;
			mq->merge_set.upp [n] = 0;
			mq->merge_set.cur [n] = empty_invlist_cur;
			mq->merge_set.next[n] = empty_invlist_next;
			mq->merge_set.skip[n] = empty_invlist_skip;
			mq->merge_set.read[n] = empty_invlist_read;
		}

		mq->ele[n] = ele; /* save for later */
		mq->merge_set.n += 1;
	}

	return 0;
}

void math_qry_release(struct math_qry *mq)
{
	if (mq->tex)
		free((char*)mq->tex);

	if (mq->optr)
		optr_release((struct optr_node*)mq->optr);

	if (mq->subpaths.n_lr_paths)
		subpaths_release(&mq->subpaths);

	if (mq->subpath_set) {
		li_free(mq->subpath_set, struct subpath_ele, ln, free(e));
	}

	for (int i = 0; i < mq->merge_set.n; i++) {
		struct math_invlist_entry_reader entry = mq->entry[i];
		if (entry.reader)
			invlist_iter_free(entry.reader);
		if (entry.fh_symbinfo)
			fclose(entry.fh_symbinfo);
	}

	if (mq->mnc.n_row > 0)
		mnc_score_free(&mq->mnc);
}

void math_qry_print(struct math_qry* mq, int print_details)
{
	printf("[math query print] %s\n", mq->tex);

	if (print_details) {
		printf("[operator tree]\n");
		optr_print(mq->optr, stdout);

		printf("[subpaths]\n");
		subpaths_print(&mq->subpaths, stdout);

		printf("[subpath set]\n");
		print_subpath_set(mq->subpath_set);
	}

	printf("[inverted lists]\n");
	for (int i = 0; i < mq->merge_set.n; i++) {
		struct subpath_ele *ele = mq->ele[i];
		struct subpath *sp = ele->dup[0];
		char path_key[MAX_DIR_PATH_NAME_LEN] = "";
		if (0 != mk_path_str(sp, ele->prefix_len, path_key)) {
			prerr("subpath too long or unexpected type.\n");
			continue;
		}

		enum math_reader_medium medium = mq->entry[i].medium;
		char *medium_str = NULL;
		switch (medium) {
		case MATH_READER_MEDIUM_INMEMO:
			medium_str = "in-memo";
			break;
		case MATH_READER_MEDIUM_ONDISK:
			medium_str = "on-disk";
			break;
		default:
			medium_str = "unknown";
		}

		printf("[%3d][%s] %s ", i, medium_str, path_key);
		printf("(pf=%u, ipf=%.2f)\n", mq->entry[i].pf, mq->ipf[i]);
	}
}
