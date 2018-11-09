#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common/common.h"
#include "postmerge/postcalls.h"
#include "postlist/math-postlist.h"
#include "search.h"
#include "math-expr-sim.h"
#include "proximity.h"

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
			l2po->pm.po[n] = math_memo_postlist(po);

			sprintf(args->po->type[n], "memo");
			args->po->weight[n] = 1;
			args->po->ele[n] = eles[i];

			printf("[%u] (in memo) %s\n", i, base_paths[i]);

		} else if (math_posting_exits(full_paths[i])) {
			po = math_posting_new_reader(full_paths[i]);
			l2po->pm.po[n] = math_disk_postlist(po);

			sprintf(args->po->type[n], "disk");
			args->po->weight[n] = 1;
			args->po->ele[n] = eles[i];

			printf("[%u] (on disk) %s\n", i, base_paths[i]);
		} else {
			printf("[%u] (empty) %s\n", i, base_paths[i]);
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
	if (po->pruner.candidate == UINT64_MAX) {
		return UINT64_MAX;
	} else {
		uint32_t candidate = (uint32_t)(po->pruner.candidate >> 32);
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

int math_l2_postlist_next(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	uint32_t prev_doc_id = (uint32_t)(po->pruner.candidate >> 32);

	do {
		po->cur_doc_id = (uint32_t)(po->pruner.candidate >> 32);
		if (po->cur_doc_id != prev_doc_id) { return 1; }

		struct math_expr_score_res expr_res;
		expr_res = math_l2_postlist_cur_score(po);

//#ifdef PRINT_RECUR_MERGING_ITEMS
//		if (cur_doc_id > 411280 && cur_doc_id < 411295) {
//			math_l2_postlist_print_cur(po);
//			printf("%u, %u score = %u \n\n",
//				cur_doc_id, expr_res.exp_id, expr_res.score);
//		}
//#endif
		if (expr_res.score) {
			/* save best expression score */
			if (expr_res.score > po->max_exp_score)
				po->max_exp_score = expr_res.score;

			/* save occurs  */
			if (po->n_occurs < MAX_HIGHLIGHT_OCCURS) {
				hit_occur_t *ho = po->occurs + po->n_occurs;
				po->n_occurs += 1;
				ho->pos = expr_res.exp_id;
#ifdef MATH_SLOW_SEARCH
				memcpy(ho->qmask, expr_res.qmask, MAX_MTREE * sizeof(uint64_t));
				memcpy(ho->dmask, expr_res.dmask, MAX_MTREE * sizeof(uint64_t));
#endif
			}
		}

		for (int i = 0; i < po->iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
			if (cur == UINT64_MAX) {
				postmerger_iter_remove(po->iter, i);
				i -= 1;
			} else if (cur == po->pruner.candidate) {
				/* forward */
				postmerger_iter_call(&po->pm, po->iter, next, i);
			}
		}

	} while (postmerger_iter_next(po->iter));

	return 0;
}

static uint32_t read_num_doc_lr_paths(struct math_l2_postlist *po)
{
	struct math_postlist_item item;

	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
//printf("i = %d, cur = %lu, candi = %lu.\n", i, cur, po->pruner.candidate);
		if (cur != UINT64_MAX && cur == po->pruner.candidate) {
			postmerger_iter_call(&po->pm, po->iter, read, i, &item, sizeof(item));
			return (item.n_lr_paths) ? item.n_lr_paths : MAX_MATH_PATHS;
		}
	}

	return 0;
}

static uint32_t get_num_doc_hit_paths(struct math_l2_postlist *po)
{
	uint32_t cnt = 0;
	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		if (cur != UINT64_MAX && cur == po->pruner.candidate)
			cnt ++;
	}

	return cnt;
}

static uint32_t set_doc_candidate(struct math_l2_postlist *po)
{
	uint64_t candidate = UINT64_MAX;
	for (int i = 0; i <= po->pruner.postlist_pivot; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		if (cur < candidate) candidate = cur;
	}
	po->pruner.candidate = candidate;
	return (uint32_t)(candidate >> 32);
}

#ifdef DEBUG_STATS_HOT_HIT
static uint64_t g_hot_hit[128];
#endif

int math_l2_postlist_pruning_next(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	uint32_t prev_doc_id = po->cur_doc_id;

	do {
		/* set candidate docID */
		po->cur_doc_id = set_doc_candidate(po);
		if (po->cur_doc_id == UINT32_MAX)
			return 0;

		/* calculate coarse score */
		uint32_t n_doc_lr_paths = read_num_doc_lr_paths(po);

#ifdef DEBUG_STATS_HOT_HIT
		uint32_t n_hit_paths = get_num_doc_hit_paths(po);
		g_hot_hit[n_hit_paths] ++;
#endif
		struct pq_align_res widest;
		// widest = math_l2_postlist_coarse_score(po, n_doc_lr_paths);
		widest = math_l2_postlist_coarse_score_v2(po, n_doc_lr_paths);

		/* push expression results */
		if (widest.width) {
			struct math_expr_score_res expr =
				math_l2_postlist_precise_score(po, &widest, n_doc_lr_paths);

			if (expr.score > po->max_exp_score)
				po->max_exp_score = expr.score;

			if (po->n_occurs < MAX_HIGHLIGHT_OCCURS) {
				hit_occur_t *ho = po->occurs + po->n_occurs;
				po->n_occurs += 1;
				ho->pos = expr.exp_id;
			}
		}

		/* forward posting lists */
		for (int i = 0; i < po->iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
			uint32_t p = po->iter->map[i];
			if (cur == UINT64_MAX || po->pruner.postlist_ref[p] <= 0) {
				/* drop this posting list completely */
				if (po->pruner.postlist_pivot >= i)
					po->pruner.postlist_pivot -= 1;
				postmerger_iter_remove(po->iter, i);
				i -= 1;
#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
				uint32_t docID = (uint32_t)(cur >> 32);
				printf("drop po#%u @ doc#%u\n", p, docID);
#endif
			} else if (cur == po->pruner.candidate) {
				/* forward */
				postmerger_iter_call(&po->pm, po->iter, next, i);
			}
		}

#if defined (DEBUG_MATH_PRUNING)
		printf("\n");
#endif
		/* collected all the expressions in this doc */
	} while (po->cur_doc_id == prev_doc_id);

	return 1;
}

static int postlist_less_than(int max_i, int len_i, int max_j, int len_j)
{
	if (max_i != max_j)
		return max_i < max_j;
	else
		return len_i < len_j;
}

static void math_l2_postlist_sort(struct math_l2_postlist *po)
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
	PTR_CAST(po, struct math_l2_postlist, po_);
	for (int i = 0; i < po->pm.n_po; i++) {
		POSTMERGER_POSTLIST_CALL(&po->pm, init, i);
	}
	po->iter = postmerger_iterator(&po->pm);
	po->n_occurs = 0;
	po->max_exp_score = 0;
	po->cur_doc_id = 0;

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
	math_l2_postlist_print_cur(po);
	math_pruner_print(&po->pruner);
#endif

#ifndef MATH_PRUNING_ENABLE
	/* read the first item. */
	math_l2_postlist_next(po_);
#endif
	return 0;
}

void math_l2_postlist_uninit(void *po_)
{
	PTR_CAST(po, struct math_l2_postlist, po_);
	for (int i = 0; i < po->pm.n_po; i++) {
		POSTMERGER_POSTLIST_CALL(&po->pm, uninit, i);
	}
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
#ifdef MATH_PRUNING_ENABLE
		math_l2_postlist_pruning_next,
#else
		math_l2_postlist_next,
#endif
		NULL /* jump */,
		math_l2_postlist_read,
		math_l2_postlist_init,
		math_l2_postlist_uninit
	};
	return ret;
}

static uint32_t
sort_occurs(hit_occur_t *dest, prox_input_t* in, int n)
{
#define CUR_POS(_in, _i) \
	_in[_i].pos[_in[_i].cur].pos
	uint32_t dest_end = 0;
	while (dest_end < MAX_HIGHLIGHT_OCCURS) {
		uint32_t i, min_idx, min_cur, min = MAX_N_POSITIONS;
		for (i = 0; i < n; i++)
			if (in[i].cur < in[i].n_pos)
				if (CUR_POS(in, i) < min) {
					min = CUR_POS(in, i);
					min_idx = i;
					min_cur = in[i].cur;
				}
		if (min == MAX_N_POSITIONS)
			/* input exhausted */
			break;
		else
			/* consume input */
			in[min_idx].cur ++;
		if (dest_end == 0 || /* first put */
		    dest[dest_end - 1].pos != min /* unique */)
			dest[dest_end++] = in[min_idx].pos[min_cur];
	}
	return dest_end;
}

static struct rank_hit
*new_hit(doc_id_t hitID, float score, prox_input_t *prox, int n)
{
	struct rank_hit *hit;
	hit = malloc(sizeof(struct rank_hit));
	hit->docID = hitID;
	hit->score = score;
	hit->occurs = malloc(sizeof(hit_occur_t) * MAX_HIGHLIGHT_OCCURS);
	hit->n_occurs = sort_occurs(hit->occurs, prox, n);
	return hit;
}

static void
topk_candidate(ranked_results_t *rk_res,
               doc_id_t docID, float score,
               prox_input_t *prox, uint32_t n)
{
	if (!priority_Q_full(rk_res) ||
	    score > priority_Q_min_score(rk_res)) {
		struct rank_hit *hit = new_hit(docID, score, prox, n);
		priority_Q_add_or_replace(rk_res, hit);
	}
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
		printf("processing query keyword `%s' ...\n", kw);
		int j = root_pm.n_po;
		if (0 == math_qry_prepare(indices, (char*)kw, &mqs[j])) {
			/* construct level-2 postlist */
			mpo[j] = math_l2_postlist(indices, mqs + j, &rk_res);
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

	/* proximity score data structure */
	prox_input_t prox[qry->len];

	// MERGE l2 posting lists here
#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
	uint64_t cnt = 0;
#endif

	foreach (iter, postmerger, &root_pm) {
		float math_score = 0.f;
		uint32_t doc_id  = 0;

		for (int i = 0; i < iter->size; i++) {
			uint64_t cur = postmerger_iter_call(&root_pm, iter, cur, i);

			if (cur != UINT64_MAX && cur == iter->min) {
				struct l2_postlist_item item;
				postmerger_iter_call(&root_pm, iter, read, i, &item, 0);

#ifdef PRINT_RECUR_MERGING_ITEMS
				printf("root[%u]: ", iter->map[i]);
				printf("%s=%u, ", STRVAR_PAIR(item.doc_id));
				printf("%s=%u, ", STRVAR_PAIR(item.part_score));
				printf("%s=%u: ", STRVAR_PAIR(item.n_occurs));
				for (int j = 0; j < item.n_occurs; j++)
					printf("%u ", item.occurs[j].pos);
				if (item.n_occurs == MAX_HIGHLIGHT_OCCURS) printf("(maximum)");
				printf("\n\n");
#endif
				doc_id     = item.doc_id;
				math_score = (float)item.part_score;
				prox_set_input(prox + i, item.occurs, item.n_occurs);

				/* advance posting iterators */
				postmerger_iter_call(&root_pm, iter, next, i);
			}
		}

		float prox_score = proximity_score(prox, iter->size);
		(void)prox_score;
		// float doc_score = prox_score + math_score;
		float doc_score = math_score;
		if (doc_score) {
			topk_candidate(&rk_res, doc_id, doc_score, prox, iter->size);
		}

#if defined(DEBUG_MERGE_LIMIT_ITERS) || defined (DEBUG_MATH_PRUNING)
		if (cnt ++ > DEBUG_MERGE_LIMIT_ITERS) {
			printf("abort (DEBUG_MERGE_LIMIT_ITERS).\n");
			break;
		}
#endif
	}

#ifdef DEBUG_STATS_HOT_HIT
	for (int i = 0; i < 128; i++) {
		if(g_hot_hit[i])
			printf("%d hits freq: %lu\n", i, g_hot_hit[i]);
	}
#endif

	// Uninitialize merger objects
	for (int j = 0; j < root_pm.n_po; j++) {
		POSTMERGER_POSTLIST_CALL(&root_pm, uninit, j);
		math_qry_free(&mqs[j]);
	}

	// Sort min-heap
	priority_Q_sort(&rk_res);
	//////////fflush(stdout);
	return rk_res;
}
