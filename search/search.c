#include "common/common.h"
#include "search.h"
#include "math-qry-struct.h"
#include "postmerge/postmerger.h"

struct math_l2_postlist {
	struct postmerger pm;
	struct postmerger_iterator iter;
};

struct on_math_paths_args {
	struct indices *indices;
	struct math_l2_postlist *po;
};

#include "postlist/postlist.h"
#include "math-index/math-index.h"

static uint64_t wrap_mem_math_postlist_cur_id(void *po)
{
	return *(uint64_t*)postlist_cur_item(po);
}

struct postmerger_postlist
math_l1_memo_post(void *po)
{
	struct postmerger_postlist ret = {
		po,
		&wrap_mem_math_postlist_cur_id,
		NULL, //&postlist_next,
		NULL,
		NULL
	};
	return ret;
}

struct postmerger_postlist
math_l1_disk_post(void *po)
{
	struct postmerger_postlist ret = {
		po,
		&math_posting_cur_id_v2,
		NULL, //&math_posting_next,
		NULL,
		NULL
	};
	return ret;
}

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
			l2po->pm.po[l2po->pm.n_po ++] = math_l1_memo_post(po);
		} else if (math_posting_exits(full_paths[i])) {
			// po = math_posting_new_reader(full_paths[i]);
			// l2po->pm.po[l2po->pm.n_po ++] = math_l1_disk_post(po);
		} else {
			/* need a NULL posting */
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

ranked_results_t
indices_run_query(struct indices* indices, struct query* qry)
{
	ranked_results_t rk_res;
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

	for (int i = 0; i < qry->len; i++) {
		const char *kw = query_get_keyword(qry, i);
		printf("%s\n", kw);

		struct math_qry_struct mqs;
		if (0 == math_qry_prepare(indices, (char*)kw, &mqs)) {
			/* construct level-2 postlist */
			struct subpaths *sp = &mqs.subpaths;
			struct math_l2_postlist po = math_l2_postlist(indices, sp);
		}
		
		math_qry_free(&mqs);
	}

	priority_Q_sort(&rk_res);
	return rk_res;
}
