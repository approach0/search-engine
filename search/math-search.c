#include "common/common.h"
#include "hashtable/u16-ht.h"
#include "tex-parser/head.h"
#include "postlist/math-postlist.h"
#include "postmerge/postcalls.h"
#include "math-index/math-index.h" /* for math_index_mk_prefix_path_str() */

#include "math-expr-sim.h"
#include "math-search.h"

void math_l2_postlist_print(struct math_l2_postlist* po)
{
	char path_str[MAX_DIR_PATH_NAME_LEN];
	char medium_str[1024];
	char pathtype_str[1024];
	for (int i = 0; i < po->pm.n_po; i++) {
		struct subpath_ele *ele = po->ele[i];
		if (ele == NULL)
			strcpy(path_str, "<empty>");
		else
			math_index_mk_prefix_path_str(ele->dup[0], ele->prefix_len,
				path_str);

		switch (po->medium[i]) {
		case MATH_POSTLIST_INMEMO:
			strcpy(medium_str, "in memo");
			break;
		case MATH_POSTLIST_ONDISK:
			strcpy(medium_str, "on disk");
			break;
		default:
			strcpy(medium_str, "empty");
		}

		switch (po->path_type[i]) {
		case MATH_PATH_TYPE_PREFIX:
			strcpy(pathtype_str, "prefix path");
			break;
		case MATH_PATH_TYPE_GENER:
			strcpy(pathtype_str, "gener path");
			break;
		default:
			strcpy(pathtype_str, "unkown type path");
		}

		printf("\t [%u] %s %s: ./%s\n", i, medium_str, pathtype_str, path_str);
	}
}

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
		/* determine path type */
		enum math_posting_type path_type = MATH_PATH_TYPE_UNKNOWN;
		if (base_paths[i][2] == 'p') path_type = MATH_PATH_TYPE_PREFIX;
		else if (base_paths[i][2] == 'g') path_type = MATH_PATH_TYPE_GENER;

		/* map posting list from cache or create on-disk posting list. */
		void *po = math_postlist_cache_find(ci.math_cache, base_paths[i]);
		int n = l2po->pm.n_po;
		if (po) {
			if (path_type == MATH_PATH_TYPE_PREFIX)
				l2po->pm.po[n] = math_memo_postlist(po);
			else
				l2po->pm.po[n] = math_memo_postlist_gener(po);

			args->po->medium[n] = MATH_POSTLIST_INMEMO;
			args->po->path_type[n] = path_type;
			args->po->ele[n] = eles[i];
			//printf("#%u (in memo) %s\n", n, base_paths[i]);

		} else if (math_posting_exits(full_paths[i])) {
			po = math_posting_new_reader(full_paths[i]);
			if (path_type == MATH_PATH_TYPE_PREFIX)
				l2po->pm.po[n] = math_disk_postlist(po);
			else
				l2po->pm.po[n] = math_disk_postlist_gener(po);

			args->po->medium[n] = MATH_POSTLIST_ONDISK;
			args->po->path_type[n] = path_type;
			args->po->ele[n] = eles[i];
			//printf("#%u (on disk) %s\n", n, base_paths[i]);

		} else {
			l2po->pm.po[n] = empty_postlist();

			args->po->medium[n] = MATH_POSTLIST_EMPTYMEM;
			args->po->path_type[n] = MATH_PATH_TYPE_UNKNOWN;
			args->po->ele[n] = NULL;
			//printf("#%u (empty) %s\n", n, base_paths[i]);

		}
		l2po->pm.n_po += 1;
	}

	return DIR_MERGE_RET_CONTINUE;
}

struct math_l2_postlist
math_l2_postlist(
	struct indices *indices,
	struct math_qry_struct* mqs,
	ranked_results_t *rk_res)
{
	struct math_l2_postlist po;
	struct add_path_postings_args args = {indices, &po};

	/* add path postings */
	postmerger_init(&po.pm);
	math_index_dir_merge(
		indices->mi, DIR_MERGE_DIRECT,
		DIR_PATHSET_PREFIX_PATH, mqs->subpath_set, mqs->n_uniq_paths,
		&add_path_postings, &args
	);

	/* save some pointers */
	po.mqs = mqs;
	po.indices = indices;
	po.rk_res = rk_res;

	return po;
}

uint64_t math_l2_postlist_cur(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	if (po->candidate == UINT64_MAX) {
		return UINT64_MAX;
	} else {
		uint32_t candidate = (uint32_t)(po->candidate >> 32);
		return candidate;
	}
}

size_t math_l2_postlist_read(void *po_, void *dest, size_t sz)
{
	PTR_CAST(item, struct l2_postlist_item, dest);
	PTR_CAST(po, struct math_l2_postlist, po_);

	item->doc_id = po->cur_doc_id;
	item->part_score = po->max_exp_score;
	item->n_occurs = po->n_occurs;

	memcpy(item->occurs, po->occurs, po->n_occurs * sizeof(hit_occur_t));

	po->max_exp_score = 0;
	po->n_occurs = 0;
	return sizeof(struct l2_postlist_item);
}

#ifdef DEBUG_STATS_HOT_HIT
static uint32_t get_num_doc_hit_paths(struct math_l2_postlist *po)
{
	uint32_t cnt = 0;
	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		if (cur != UINT64_MAX && cur == po->candidate)
			cnt ++;
	}

	return cnt;
}
#endif

static uint32_t set_doc_candidate(struct math_l2_postlist *po)
{
	uint64_t candidate = UINT64_MAX;
#ifdef MATH_PRUNING_DISABLE_JUMP
	for (int i = 0; i < po->iter->size; i++) {
#else
	for (int i = 0; i <= po->pruner.postlist_pivot; i++) {
#endif
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		if (cur < candidate) candidate = cur;
	}
	po->candidate = candidate;
	return (uint32_t)(candidate >> 32);
}

static uint32_t read_num_doc_lr_paths(struct math_l2_postlist *po)
{
	struct math_postlist_gener_item item;

	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		if (cur != UINT64_MAX && cur == po->candidate) {
			postmerger_iter_call(&po->pm, po->iter, read, i, &item, sizeof(item));
			return (item.n_lr_paths) ? item.n_lr_paths : MAX_MATH_PATHS;
		}
	}

	return 0;
}

#ifdef DEBUG_STATS_HOT_HIT
static uint64_t g_hot_hit[128];
#endif

int math_l2_postlist_next(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	struct math_pruner *pruner = &po->pruner;
	float threshold = pruner->init_threshold;

	/* update threshold value */
	if (priority_Q_full(po->rk_res))
		threshold = MAX(priority_Q_min_score(po->rk_res), threshold);

#ifndef MATH_PRUNING_DISABLE_JUMP
	/* if threshold has been updated */
	if (unlikely(threshold != pruner->prev_threshold)) {
		/* try to "lift up" the pivot */
		int i, sum = 0;
		for (i = po->iter->size - 1; i >= 0; i--) {
			uint32_t pid = po->iter->map[i];
			int qmw = pruner->postlist_max[pid];
			sum += qmw;
			if (pruner->upp[sum] > threshold)
				break;
			/* otherwise this posting list can be skip-only */
		}

		pruner->postlist_pivot = i;
		pruner->prev_threshold = threshold;
	}
#endif

	/* Iterate through future math items until cur goes to even future,
	 * i.e., cur != future. Then update future ID. */
	po->cur_doc_id = po->future_doc_id;
	do {
		/* set candidate docID */
		po->cur_doc_id = set_doc_candidate(po);

		if (po->cur_doc_id == UINT32_MAX) {
			po->future_doc_id = UINT32_MAX;
			return 0;

		} else if (po->cur_doc_id != po->future_doc_id) {
			/* cur is now even future than future. */
			uint32_t save = po->cur_doc_id;
			po->cur_doc_id = po->future_doc_id;
			po->future_doc_id = save;
			return 1;
		}

		uint64_t candidate = po->candidate;

#ifndef MATH_PRUNING_DISABLE_JUMP
		/* followers jump to candidate */
		int pivot = po->pruner.postlist_pivot;
		for (int i = pivot + 1; i < po->iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);

			if (cur < candidate) {
				postmerger_iter_call(&po->pm, po->iter, jump, i, candidate);
#ifdef DEBUG_MERGE_SKIPPING
				uint64_t cur_ = postmerger_iter_call(&po->pm, po->iter, cur, i);
				printf("target: %u, po[%d] jumps: %u --> %u.\n",
					   candidate >> 32, i, cur >> 32, cur_ >> 32);
#endif
			}
		}
#endif


#ifdef DEBUG_STATS_HOT_HIT
		uint32_t n_hit_paths = get_num_doc_hit_paths(po);
		g_hot_hit[n_hit_paths] ++;
#endif
		struct pq_align_res widest;
		widest = math_l2_postlist_widest_estimate(po, threshold);

		/* push expression results */
		if (widest.width) {
			uint32_t n_doc_lr_paths = read_num_doc_lr_paths(po);

			struct math_expr_score_res expr =
				math_l2_postlist_precise_score(po, &widest, n_doc_lr_paths);

			if (expr.score > po->max_exp_score)
				po->max_exp_score = expr.score;

			if (po->n_occurs < MAX_HIGHLIGHT_OCCURS) {
				hit_occur_t *ho = po->occurs + po->n_occurs;
				po->n_occurs += 1;
				ho->pos = expr.exp_id;
#ifdef HIGHLIGHT_MATH_ALIGNMENT
				/* copy highlight mask */
				ho->qmask[0] = widest.qmask;
				ho->dmask[0] = widest.dmask;
#endif
			}
		}

		/* forward posting lists */
		for (int i = 0; i < po->iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
			uint32_t p = po->iter->map[i];

			if (cur == UINT64_MAX
#ifdef MATH_PRUNING_ENABLE
			|| po->pruner.postlist_ref[p] <= 0
#endif
			) {
				/* drop this posting list completely */
				if (po->pruner.postlist_pivot >= i)
					po->pruner.postlist_pivot -= 1;
				postmerger_iter_remove(po->iter, i);
				i -= 1;
#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING) || defined (DEBUG_MATH_SCORE_INSPECT)
				uint32_t docID = (uint32_t)(cur >> 32);
				printf("drop po#%u @ doc#%u\n", p, docID);
				// math_pruner_print(&po->pruner);
#endif
			} else if (cur == candidate) {
				/* forward */
				postmerger_iter_call(&po->pm, po->iter, next, i);
			}
		}

		/* collected all the expressions in this doc */
	} while (true);

	return 1;
}

/* for debug purpose */
void math_l2_cur_print(struct math_l2_postlist*, uint64_t, float);
#ifdef DEBUG_MATH_SCORE_INSPECT
int score_inspect_filter(doc_id_t, struct indices*);
#endif

int math_l2_postlist_one_by_one_through(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);

//	math_pruner_print(&po->pruner);
//	math_l2_postlist_print_cur(po);
//	printf("\n");

	for (int i = 0; i < po->iter->size; i++) {
		uint32_t pid = po->iter->map[i];
		enum math_posting_type pt = po->path_type[pid];

		printf("Going through posting list #%u (%s) ...\n", pid,
			(pt == MATH_PATH_TYPE_PREFIX) ? "prefix" : "gener");
		while (1) {
			uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
			if ((cur >> 32) == UINT32_MAX) break;

#ifdef DEBUG_MATH_SCORE_INSPECT
			if (score_inspect_filter(cur >> 32, po->indices)) {
				printf("it is in po#%u iter[%d]\n", pid, i);
				break;
			}
#else
			/* print math postlist item */
			struct math_postlist_gener_item item;
			postmerger_iter_call(&po->pm, po->iter, read, i, &item, sizeof(item));
			math_postlist_print_item(&item, pt == MATH_PATH_TYPE_GENER, trans_symbol);
#endif

			postmerger_iter_call(&po->pm, po->iter, next, i);
		}

		printf("\n");
	}

	po->candidate = UINT64_MAX;
	return 0;
}

static int postlist_less_than(int max_i, int len_i, int max_j, int len_j)
{
	if (max_i != max_j)
		/* descending refMax value, according to
		 * the pruning algorithm. */
		return max_i < max_j;
	else
		/* ascending posting list length, so that
		 * longer one gets dropped or skipped. */
		return len_i > len_j;
}

void math_l2_postlist_sort(struct math_l2_postlist *po)
{
	struct math_pruner *pruner = &po->pruner;
	for (int i = 0; i < po->iter->size; i++) {
		for (int j = i + 1; j < po->iter->size; j++) {
			int pid_i = po->iter->map[i];
			int pid_j = po->iter->map[j];
			int max_i = pruner->postlist_max[pid_i];
			int max_j = pruner->postlist_max[pid_j];
			int len_i = pruner->postlist_len[pid_i];
			int len_j = pruner->postlist_len[pid_j];
			if (postlist_less_than(max_i, len_i, max_j, len_j)) {
				/* swap */
				po->iter->map[i] = pid_j;
				po->iter->map[j] = pid_i;
			}
		}
	}
}

int math_l2_postlist_init(void *po_)
{
	/* initialize inner pm */
	PTR_CAST(po, struct math_l2_postlist, po_);
	for (int i = 0; i < po->pm.n_po; i++) {
		POSTMERGER_POSTLIST_CALL(&po->pm, init, i);
	}

	/* allocate iterator */
	po->iter = postmerger_iterator(&po->pm);

	/* setup current doc-level item */
	po->cur_doc_id = 0;
	po->future_doc_id = 0;
	po->max_exp_score = 0;
	po->n_occurs = 0;

	/* initialize candidate */
	po->candidate = 0;

	/* setup pruning structures */
	uint32_t n_qnodes = po->mqs->n_qry_nodes;
	uint32_t n_postings = po->pm.n_po;
	uint32_t qw = po->mqs->subpaths.n_lr_paths;
	math_pruner_init(&po->pruner, n_qnodes, po->ele, n_postings);
	math_pruner_init_threshold(&po->pruner, qw);
	math_pruner_precalc_upperbound(&po->pruner, qw);

	/* sort path posting lists by max values */
	math_l2_postlist_sort(po);

#ifdef DEBUG_PRINT_QRY_STRUCT
	math_pruner_print(&po->pruner);
	math_l2_postlist_print_cur(po);
#endif

	// printf("%u postings.\n", po->iter->size);
	return 0;
}

void math_l2_postlist_uninit(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);

	/* release inner pm */
	for (int i = 0; i < po->pm.n_po; i++) {
		POSTMERGER_POSTLIST_CALL(&po->pm, uninit, i);
	}

	/* release iterator */
	postmerger_iter_free(po->iter);

	/* release pruning structures */
	math_pruner_free(&po->pruner);
}

struct postmerger_postlist
postmerger_math_l2_postlist(struct math_l2_postlist *po)
{
	struct postmerger_postlist ret = {
		po,
		math_l2_postlist_cur,
		math_l2_postlist_next,
		// math_l2_postlist_one_by_one_through, /* for debug */
		NULL /* jump */,
		math_l2_postlist_read,
		math_l2_postlist_init,
		math_l2_postlist_uninit
	};

	return ret;
}
