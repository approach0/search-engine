#include "common/common.h"
#include "search.h"
#include "math-qry-struct.h"
#include "postmerge/postcalls.h"

struct math_l2_postlist {
	struct postmerger pm;
	struct postmerger_iterator iter;
};

struct on_math_paths_args {
	struct indices *indices;
	struct math_l2_postlist *po;
};

static enum dir_merge_ret
on_math_paths(
	char (*full_paths)[MAX_MERGE_DIRS], char (*base_paths)[MAX_MERGE_DIRS],
	struct subpath_ele **eles, uint32_t n_eles, uint32_t level, void *_args)
{
	PTR_CAST(args, struct on_math_paths_args, _args);
	struct postlist_cache ci = args->indices->ci;
	struct math_l2_postlist *l2po = args->po;

	for (uint32_t i = 0; i < n_eles; i++) {
		void *po = math_postlist_cache_find(ci.math_cache, base_paths[i]);
		if (po) {
			l2po->pm.po[l2po->pm.n_po ++] = math_memo_postlist(po);
		} else if (math_posting_exits(full_paths[i])) {
			po = math_posting_new_reader(full_paths[i]); /* need to be freed */
			l2po->pm.po[l2po->pm.n_po ++] = math_disk_postlist(po);
		} else {
			; /* NULL */
		}
		printf("%s\n", full_paths[i]);
	}
	printf("\n");
	return DIR_MERGE_RET_CONTINUE;
}

struct math_l2_postlist
math_l2_postlist(struct indices *indices, struct subpaths *subpaths)
{
	struct math_l2_postlist po;
	struct on_math_paths_args args = {indices, &po};
	postmerger_init(&po.pm);
	math_index_dir_merge(
		indices->mi, DIR_MERGE_DIRECT, DIR_PATHSET_PREFIX_PATH,
		subpaths, on_math_paths, &args
	);
	po.iter = postmerger_iterator(&po.pm);
	return po;
}

struct postmerger_postlist
postmerger_math_l2_postlist(struct math_l2_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		NULL,
		NULL
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

	for (int i = 0; i < qry->len; i++) {
		const char *kw = query_get_keyword(qry, i);
		printf("%s\n", kw);
		int j = root_pm.n_po;

		if (0 == math_qry_prepare(indices, (char*)kw, &mqs[j])) {
			/* construct level-2 postlist */
			struct subpaths *sp = &(mqs[j].subpaths);
			mpo[j] = math_l2_postlist(indices, sp);
			root_pm.po[j] = postmerger_math_l2_postlist(mpo + j);
			root_pm.n_po += 1;
		}
	}
	
	for (int j = 0; j < root_pm.n_po; j++) {
		POSTMERGER_POSTLIST_CALL(&root_pm, init, j);
	}

	// MERGE here

	for (int j = 0; j < root_pm.n_po; j++) {
		POSTMERGER_POSTLIST_CALL(&root_pm, uninit, j);
		math_qry_free(&mqs[j]);
	}

	priority_Q_sort(&rk_res);
	return rk_res;
}
