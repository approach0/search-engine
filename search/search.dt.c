#include "common/common.h"
#include "search.h"
#include "math-qry-struct.h"
#include "postmerge/postcalls.h"

struct math_l2_postlist {
	struct postmerger pm;
	struct postmerger_iterator iter;
	uint32_t docID;
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
			printf("[memo] %s\n", base_paths[i]);
			l2po->pm.po[l2po->pm.n_po ++] = math_memo_postlist(po);
		} else if (math_posting_exits(full_paths[i])) {
			printf("[disk] %s\n", base_paths[i]);
			po = math_posting_new_reader(full_paths[i]);
			l2po->pm.po[l2po->pm.n_po ++] = math_disk_postlist(po);
		} else {
			printf("[empty] %s\n", base_paths[i]);
			; // empty posting list?
		}
	}
	printf("\n");
	return DIR_MERGE_RET_CONTINUE;
}

struct math_l2_postlist
create_math_l2_postlist(struct indices *indices, struct subpaths *subpaths)
{
	struct math_l2_postlist po;
	struct on_math_paths_args args = {indices, &po};
	postmerger_init(&po.pm);
	math_index_dir_merge(
		indices->mi, DIR_MERGE_DIRECT, DIR_PATHSET_PREFIX_PATH,
		subpaths, on_math_paths, &args
	);
	return po;
}

int math_l2_postlist_next(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	do {
		//uint32_t min_docID = (uint32_t)po->iter.min;

		for (int i = 0; i < po->iter.size; i++) {
			uint64_t cur = postmerger_iter_call(&po->pm, &po->iter, cur, i);
			uint32_t docID = (uint32_t)(cur >> 32);
			uint32_t expID = (uint32_t)(cur >> 0);

			printf("[%u] -> [%u]: %u,%u \n", i, po->iter.map[i], docID, expID);

			if (cur == UINT64_MAX) {
				postmerger_iter_remove(&po->iter, i);
				i -= 1;
			} else if (cur == po->iter.min) {
				postmerger_iter_call(&po->pm, &po->iter, next, i);
			}
		}
		printf("\n");

	} while (postmerger_iter_next(&po->pm, &po->iter));

	return 0;
}

uint64_t math_l2_postlist_cur(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	if (po->iter.min == UINT64_MAX) {
		return UINT64_MAX;
	} else {
		uint32_t min_docID = (uint32_t)po->iter.min;
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

	for (int i = 0; i < qry->len; i++) {
		const char *kw = query_get_keyword(qry, i);
		printf("%s\n", kw);
		int j = root_pm.n_po;

		if (0 == math_qry_prepare(indices, (char*)kw, &mqs[j])) {
			/* construct level-2 postlist */
			struct subpaths *sp = &(mqs[j].subpaths);
			mpo[j] = create_math_l2_postlist(indices, sp);
			root_pm.po[j] = postmerger_math_l2_postlist(mpo + j);
			root_pm.n_po += 1;
		} else {
			; // empty posting list?
		}
	}

	for (int j = 0; j < root_pm.n_po; j++) {
		POSTMERGER_POSTLIST_CALL(&root_pm, init, j);
	}

	// MERGE here
	foreach (iter, postmerger, &root_pm) {
		for (int i = 0; i < iter.size; i++) {
			uint64_t cur = postmerger_iter_call(&root_pm, &iter, cur, i);

			printf("root[%u]: %u \n", iter.map[i], (uint32_t)cur);
			if (cur == iter.min)
				postmerger_iter_call(&root_pm, &iter, next, i);
		}
	}

	for (int j = 0; j < root_pm.n_po; j++) {
		POSTMERGER_POSTLIST_CALL(&root_pm, uninit, j);
		math_qry_free(&mqs[j]);
	}

	priority_Q_sort(&rk_res);
	return rk_res;
}
