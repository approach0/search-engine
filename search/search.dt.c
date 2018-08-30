#include "common/common.h"
#include "postmerge/postcalls.h"
#include "search.h"
#include "math-expr-sim.h"

struct add_path_postings_args {
	struct indices *indices;
	struct math_l2_postlist *po;
};

static enum dir_merge_ret
add_path_postings( /* add (l1) path posting lists into l2 posting list */
	char (*full_paths)[MAX_MERGE_DIRS], char (*base_paths)[MAX_MERGE_DIRS],
	struct subpath_ele **eles, uint32_t n_eles, uint32_t level, void *_args)
{
	PTR_CAST(args, struct add_path_postings_args, _args);
	struct postlist_cache ci = args->indices->ci;
	struct math_l2_postlist *l2po = args->po;

	for (uint32_t i = 0; i < n_eles; i++) {
		void *po = math_postlist_cache_find(ci.math_cache, base_paths[i]);
		int n = l2po->pm.n_po;
		if (po) {
			printf("[memo] [%u] %s\n", i, base_paths[i]);
			l2po->pm.po[n] = math_memo_postlist(po);

			sprintf(args->po->type[n], "memo");
			args->po->weight[n] = 1;
			args->po->ele[n] = eles[i];

		} else if (math_posting_exits(full_paths[i])) {
			printf("[disk] [%u] %s\n", i, base_paths[i]);
			po = math_posting_new_reader(full_paths[i]);
			l2po->pm.po[n] = math_disk_postlist(po);

			sprintf(args->po->type[n], "disk");
			args->po->weight[n] = 1;
			args->po->ele[n] = eles[i];

		} else {
			printf("[empty] [%u] %s\n", i, base_paths[i]);
			l2po->pm.po[n] = empty_postlist(NULL);

			sprintf(args->po->type[n], "empty");
			args->po->weight[n] = 0;
			args->po->ele[n] = NULL;
		}
		l2po->pm.n_po += 1;
	}
	printf("\n");
	return DIR_MERGE_RET_CONTINUE;
}

struct math_l2_postlist
math_l2_postlist(struct indices *indices, struct math_qry_struct* mqs)
{
	struct math_l2_postlist po;
	struct add_path_postings_args args = {indices, &po};
	postmerger_init(&po.pm);
	math_index_dir_merge(
		indices->mi, DIR_MERGE_DIRECT, DIR_PATHSET_PREFIX_PATH,
		&mqs->subpaths, add_path_postings, &args
	);
	po.mqs = mqs;
	po.indices = indices;
	return po;
}

int math_l2_postlist_next(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	uint32_t prev_min_docID = (uint32_t)(po->iter.min >> 32);
	do {
		uint32_t cur_min_docID = (uint32_t)(po->iter.min >> 32);
		if (cur_min_docID != prev_min_docID) {
			return 1;
		}

		math_l2_postlist_print_cur(po);

		struct math_expr_score_res expr_res;
		expr_res = math_l2_postlist_cur_score(po);
		(void)(expr_res);

		printf("score = %u \n\n", expr_res.score);

		for (int i = 0; i < po->iter.size; i++) {
			uint64_t cur = postmerger_iter_call(&po->pm, &po->iter, cur, i);
			if (cur == UINT64_MAX) {
				postmerger_iter_remove(&po->iter, i);
				i -= 1;
			} else if (cur == po->iter.min) {
				/* forward */
				postmerger_iter_call(&po->pm, &po->iter, next, i);
			}
		}

	} while (postmerger_iter_next(&po->pm, &po->iter));

	return 0;
}

uint64_t math_l2_postlist_cur(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	if (po->iter.min == UINT64_MAX) {
		return UINT64_MAX;
	} else {
		uint32_t min_docID = (uint32_t)(po->iter.min >> 32);
		return min_docID;
	}
}

int math_l2_postlist_init(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	for (int i = 0; i < po->pm.n_po; i++) {
		POSTMERGER_POSTLIST_CALL(&po->pm, init, i);
	}
	po->iter = postmerger_iterator(&po->pm);
	return 0;
}

void math_l2_postlist_uninit(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	for (int i = 0; i < po->pm.n_po; i++) {
		POSTMERGER_POSTLIST_CALL(&po->pm, uninit, i);
	}
}

struct postmerger_postlist
postmerger_math_l2_postlist(struct math_l2_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		math_l2_postlist_cur,
		math_l2_postlist_next,
		NULL /* jump */,
		NULL /* read */,
		math_l2_postlist_init,
		math_l2_postlist_uninit
	};
	return ret;
}

ranked_results_t
indices_run_query(struct indices* indices, struct query* qry)
{
	struct postmerger root_pm;
	postmerger_init(&root_pm);

	ranked_results_t rk_res;
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

	struct math_qry_struct  mqs[qry->len];
	struct math_l2_postlist mpo[qry->len];

	// Create merger objects
	for (int i = 0; i < qry->len; i++) {
		const char *kw = query_get_keyword(qry, i);
		printf("%s\n", kw);
		int j = root_pm.n_po;
		if (0 == math_qry_prepare(indices, (char*)kw, &mqs[j])) {
			/* construct level-2 postlist */
			mpo[j] = math_l2_postlist(indices, mqs + j);
			root_pm.po[j] = postmerger_math_l2_postlist(mpo + j);
		} else {
			root_pm.po[j] = empty_postlist(NULL);
		}
		root_pm.n_po += 1;
	}

	// Initialize merger objects
	for (int j = 0; j < root_pm.n_po; j++) {
		POSTMERGER_POSTLIST_CALL(&root_pm, init, j);
	}

	// MERGE here
	foreach (iter, postmerger, &root_pm) {
		for (int i = 0; i < iter.size; i++) {
			uint64_t cur = postmerger_iter_call(&root_pm, &iter, cur, i);

			printf("root[%u]: %lu \n", iter.map[i], cur);
			if (cur == iter.min)
				postmerger_iter_call(&root_pm, &iter, next, i);
		}
	}

	// Uninitialize merger objects
	for (int j = 0; j < root_pm.n_po; j++) {
		POSTMERGER_POSTLIST_CALL(&root_pm, uninit, j);
		math_qry_free(&mqs[j]);
	}

	// Sort min-heap
	priority_Q_sort(&rk_res);
	return rk_res;
}
