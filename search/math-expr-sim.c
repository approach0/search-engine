#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "common/common.h"
#include "tex-parser/head.h"
#include "indexer/config.h"
#include "indexer/index.h"
#include "postlist/math-postlist.h"

#include "config.h"
#include "search-utils.h"
#include "math-prefix-qry.h"
#include "math-expr-sim.h"

void math_expr_sim_factors_print(struct math_expr_sim_factors *factor)
{
	printf("%s: %u\n", STRVAR_PAIR(factor->qry_lr_paths));
	printf("%s: %u\n", STRVAR_PAIR(factor->doc_lr_paths));
	for (int i = 0; i < factor->k; i++) {
		printf("tree#%d: width=%u, qr=%u, dr=%u\n", i,
			factor->align_res[i].width,
			factor->align_res[i].qr, factor->align_res[i].dr);
	}
	printf("%s: %u\n", STRVAR_PAIR(factor->mnc_score));
}

#ifdef DEBUG_MATH_SCORE_INSPECT
int score_inspect_filter(doc_id_t doc_id, struct indices *indices)
{
	size_t url_sz;
	int ret = 0;
//return 1;
	char *url = get_blob_string(indices->url_bi, doc_id, 0, &url_sz);
	char *txt = get_blob_string(indices->txt_bi, doc_id, 1, &url_sz);
//	if (0 == strcmp(url, "Opening_(morphology):7") ||
//	    0 == strcmp(url, "Mathematical_morphology:24")) {

//	if (doc_id == 308876 || doc_id == 470478) {

//	if (doc_id == 368782) {

	if (0 == strcmp(url, "Riemann–Stieltjes_integral:9")) {

		printf("%s: doc %u, url: %s\n", __func__, doc_id, url);
		// printf("%s \n", txt);
		ret = 1;
	}

	free(url);
	free(txt);
	return ret;
}
#endif

static void
math_expr_set_score__fixed(
	struct math_expr_sim_factors* factor,
	struct math_expr_score_res* hit)
{
	hit->score = 1.f;
}

static void
math_expr_set_score__multi_tree(
	struct math_expr_sim_factors* factor,
	struct math_expr_score_res* hit)
{
	struct pq_align_res *ar = factor->align_res;
	uint32_t jo = factor->joint_nodes;
	uint32_t lcs = factor->lcs; (void)lcs;
	float stj = (float)jo; (void)stj;
	uint32_t qnn = factor->qry_nodes; (void)qnn;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));

	const float theta = 0.05f;
	const float alpha = 0.0f;
	const float beta[5] = {
		1.0f,
		0.f,
		0.f,
		0.f,
		0.f
	};

	float st = 0;
	for (int i = 0; i < MAX_MTREE; i++) {
		float l = (float)ar[i].width;
		float o = (float)ar[i].height;
		if (alpha == 0.f)
			st += (beta[i] * (alpha * o + (1.f - alpha) * l)) / (float)qn;
		else
			st += (beta[i] * (alpha * o + (1.f - alpha) * l)) / (float)qnn;
	}

//	float st = (0.75f * st0 + 0.20f * st1 + 0.05f * st2) / (float)qnn; /* experiment */

	float fmeasure = (st * sy) / (st + sy);
	float score = fmeasure * ((1.f - theta) + theta * (1.f / logf(1.f + dn)));

	hit->score = score;
}

static void
math_expr_set_score__opd_only(
	struct math_expr_sim_factors* factor,
	struct math_expr_score_res* hit)
{
	struct pq_align_res *ar = factor->align_res;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));

	const float theta = 0.05f;
	float st = (float)ar[0].width / (float)qn;

	float fmeasure = (st * sy) / (st + sy);
	float score = fmeasure * ((1.f - theta) + theta * (1.f / logf(1.f + dn)));

	hit->score = score;
}

float math_expr_score_upperbound(int width, int qw)
{
	const float theta = 0.05f;
	const float dw = 1.f;
	float st = (float)width / (float)qw;
	float fmeasure = st / (st + 1.f);
	return fmeasure * ((1.f - theta) + theta * (1.f / logf(1.f + dw)));
}
float math_expr_score_lowerbound(int width, int qw)
{
	const float theta = 0.05f;
	const float dw = (float)MAX_LEAVES;
	float st = (float)width / (float)qw;
	float fmeasure = (st * 0.5f) / (st + 0.5f);
	return fmeasure * ((1.f - theta) + theta * (1.f / logf(1.f + dw)));
}

void
math_expr_set_score(struct math_expr_sim_factors* factor,
                    struct math_expr_score_res* hit)
{
#ifndef MATH_SYMBOLIC_SCORING_ENABLE
	factor->mnc_score = 0;
#endif

#ifndef MATH_PRUNING_ENABLE
	math_expr_set_score__multi_tree(factor, hit);
#else
	math_expr_set_score__opd_only(factor, hit);
#endif
}

//int string_longest_common_substring(enum symbol_id *str1, enum symbol_id *str2)
//{
//	int (*DP)[MAX_LEAVES] = calloc(MAX_LEAVES, MAX_LEAVES * sizeof(int));
//	int lcs = 0;
//	int i, j;
//	for (i = 0; i < MAX_LEAVES; i++) {
//		for (j = 0; j < MAX_LEAVES; j++) {
//			if (i == 0 || j == 0) {
//				DP[i][j] = 0;
//			} else if (str1[i-1] == str2[j-1] &&
//					   str1[i-1] != 0) {
//				DP[i][j] = DP[i-1][j-1] + 1;
//				if (DP[i][j] > lcs)
//					lcs = DP[i][j];
//			} else {
//				DP[i][j] = 0;
//			}
//		}
//	}
//	free(DP);
//
//	return lcs;
//}
//static int
//prefix_symbolseq_similarity(uint64_t cur_min, struct postmerge* pm)
//{
//	int i, j, k;
//
//	enum symbol_id querystr[MAX_LEAVES] = {0};
//	enum symbol_id candistr[MAX_LEAVES] = {0};
//
//	for (i = 0; i < pm->n_postings; i++) {
//		PTR_CAST(item, struct math_postlist_item, POSTMERGE_CUR(pm, i));
//		PTR_CAST(mepa, struct math_extra_posting_arg, pm->posting_args[i]);
//
//		if (pm->curIDs[i] == cur_min) {
//			for (j = 0; j <= mepa->ele->dup_cnt; j++) {
//				uint32_t qn = mepa->ele->dup[j]->leaf_id; /* use leaf_id for order ID */
//				enum symbol_id qs = mepa->ele->dup[j]->lf_symbol_id;
//				querystr[qn - 1] = qs;
//
//				for (k = 0; k < item->n_paths; k++) {
//					candistr[item->leaf_id[k] - 1] = item->lf_symb[k];
//				}
//			}
//		} /* end if */
//	} /* end for */
//
//	return string_longest_common_substring(querystr, candistr);
//////	for (i = 0; i < MAX_LEAVES; i++) {
//////		printf("%s ", trans_symbol(querystr[i]));
//////	} printf("\n");
//////	for (i = 0; i < MAX_LEAVES; i++) {
//////		printf("%s ", trans_symbol(candistr[i]));
//////	} printf("\n");
//////	printf("lcs = %u\n", lcs);
//}

void math_l2_postlist_print_cur(struct math_l2_postlist *po)
{
	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		uint32_t docID = (uint32_t)(cur >> 32);
		uint32_t expID = (uint32_t)(cur >> 0);

		uint32_t orig = po->iter->map[i];
		printf("po#%u under iter[%u]: doc#%u,exp#%u \n", orig, i, docID, expID);
	}
}

/* symbolset similarity calculation, single-tree only. */
static mnc_score_t
#ifdef DEBUG_MATH_SCORE_INSPECT
math_l2_postlist_cur_symbol_sim(struct math_l2_postlist *po,
	struct pq_align_res *ar, int inspect)
#else
math_l2_postlist_cur_symbol_sim(struct math_l2_postlist *po,
	struct pq_align_res *ar)
#endif
{
	struct mnc_match_t match;
	struct math_postlist_gener_item item;
	struct math_postlist_item *_item;
	mnc_reset_docs();

	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);

		if (cur != UINT64_MAX && cur == po->candidate) {
			uint32_t orig = po->iter->map[i];
			struct subpath_ele *ele = po->ele[orig];
			enum math_posting_type pt = po->path_type[orig];

			for (uint32_t j = 0; j <= ele->dup_cnt; j++) {
				uint32_t qr = ele->rid[j];
				uint32_t ql = ele->dup[j]->path_id; /* use path_id for mnc_score */
				if (qr == ar->qr) {
					postmerger_iter_call(&po->pm, po->iter, read, i,
					                     &item, sizeof(item));
					for (uint32_t k = 0; k < item.n_paths; k++) {
						uint32_t dr = item.subr_id[k];

						if (dr == ar->dr) {
							struct mnc_ref ref = {
								item.tr_hash[k],
								item.op_hash[k]
							};

							if (MATH_PATH_TYPE_PREFIX == pt) {
								_item = (struct math_postlist_item*)&item;
								uint32_t dl = _item->leaf_id[k];
								mnc_doc_add_rele(ql - 1, dl - 1, ref);
							} else {
								uint64_t dls = item.wild_leaves[k];
								mnc_doc_add_reles(ql - 1, dls, ref);
							}
						}
					}
				}
			} /* end for */
		} /* end if */
	} /* end for */
#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect) {
		match = mnc_match_debug();
	} else
#endif

	match = mnc_match();

#ifdef HIGHLIGHT_MATH_ALIGNMENT
	ar->qmask = match.qry_paths;
	ar->dmask = match.doc_paths;
#endif
	return match.score;
}

////////////////////  pruning functions ////////////////////////////////
void math_l2_cur_print(struct math_l2_postlist *po,
                       uint64_t candidate, float threshold)
{
	struct math_pruner *pruner = &po->pruner;
	printf("doc#%lu, pivot = %d/%u, threshold = %.3f.\n", candidate >> 32,
	        pruner->postlist_pivot, po->iter->size, threshold);
	for (int i = 0; i < po->iter->size; i++) {
		uint32_t pid = po->iter->map[i];
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		if (i > pruner->postlist_pivot || cur == candidate) {
			printf("%s ", (cur == candidate) ? "hit: " : "skip:");
			printf("iter[%u] ", i);
			math_pruner_print_postlist(pruner, pid);
			uint32_t docID = (uint32_t)(cur >> 32);
			uint32_t expID = (uint32_t)(cur >> 0);
			printf(" [doc#%u, exp#%u]\n", docID, expID);
		}
#ifdef DEBUG_MATH_SCORE_INSPECT
		else {
			printf("pass: [%u] ", i);
			math_pruner_print_postlist(pruner, pid);
			uint32_t docID = (uint32_t)(cur >> 32);
			uint32_t expID = (uint32_t)(cur >> 0);
			printf(" [doc#%u, exp#%u]\n", docID, expID);
		}
#endif
	}
	printf("\n");
}

struct math_expr_score_res
math_l2_postlist_precise_score(struct math_l2_postlist *po,
                               struct pq_align_res *widest,
                               uint32_t dn)
{
	struct math_expr_score_res expr;
	uint32_t r_cnt = 0;

	/* get docID and exprID */
	uint64_t cur = po->candidate;
	P_CAST(p, struct math_postlist_item, &cur);
	expr.doc_id = p->doc_id;
	expr.exp_id = p->exp_id;

	/* get symbolic score */
#ifdef DEBUG_MATH_SCORE_INSPECT
	int inspect = score_inspect_filter(p->doc_id, po->indices);
	mnc_score_t sym_sim = math_l2_postlist_cur_symbol_sim(po, widest, inspect);
#else
	mnc_score_t sym_sim = math_l2_postlist_cur_symbol_sim(po, widest);
#endif

	/* get single tree overall score */
	const uint32_t k = 1;
	uint32_t qn = po->mqs->subpaths.n_lr_paths;
	struct math_expr_sim_factors factors = {
		sym_sim, 0 /* search depth*/, qn, dn, widest, k,
		r_cnt, 0 /* lcs */, po->mqs->n_qry_nodes};
	math_expr_set_score(&factors, &expr);

#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect) {
		/* print posting list hits */
		math_l2_cur_print(po, po->candidate, 0);

		math_expr_sim_factors_print(&factors);
#ifdef HIGHLIGHT_MATH_ALIGNMENT
		printf("alignment: qry: 0x%lx, doc: 0x%lx.\n",
		       widest->qmask, widest->dmask);
#endif
		printf("doc#%u, exp#%u, final score: %f\n",
		       expr.doc_id, expr.exp_id, expr.score);
		printf("\n");
	}
#endif

	return expr;
}

struct q_node_match {
	int dr, max;
};

static struct q_node_match
#ifdef DEBUG_MATH_SCORE_INSPECT
calc_q_node_match(struct math_l2_postlist *po, struct pruner_node *q_node,
                  uint32_t widest, int inspect)
#else
calc_q_node_match(struct math_l2_postlist *po, struct pruner_node *q_node,
                  uint32_t widest)
#endif
{
	struct math_pruner *pruner = &po->pruner;
	struct q_node_match ret = {0};
	u16_ht_reset(&pruner->accu_ht, 0);

#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect) printf("qr#%d/%d:\n", q_node->secttr[0].rnode, q_node->width);
#endif

#ifdef MATH_PRUNING_SECTR_DROP_ENABLE
	int upbound = q_node->width;
#endif

	/* for each sector tree rooted at q_node ... */
	for (int i = 0; i < q_node->n; i++) {
		struct math_postlist_gener_item item;
		int pid = q_node->postlist_id[i];
		int qsw = q_node->secttr[i].width;
		uint64_t cur = POSTMERGER_POSTLIST_CALL(&po->pm, cur, pid);

#ifdef DEBUG_MATH_SCORE_INSPECT
		if (inspect) printf("\t secttr[%d] width: %d -> po#%d\n", i, qsw, pid);
#endif

		/* skip non-hit posting lists */
		if (cur == UINT64_MAX || cur != po->candidate) continue;

		/* read document posting list item */
		POSTMERGER_POSTLIST_CALL(&po->pm, read, pid, &item, sizeof(item));

		/* calculate match counter vector */
		u16_ht_reset(&pruner->sect_ht, 0);

#ifdef DEBUG_MATH_SCORE_INSPECT
		if (inspect) {
			for (int _ = 0; _ < item.n_paths; _++) {
				printf("\t + min(%d, dr=%d)\n", qsw, item.subr_id[_]);
			}
		}
#endif

		for (int j = 0; j < item.n_paths; j++) {
			int dr = item.subr_id[j];
			int dr_sect_cnt = u16_ht_lookup(&pruner->sect_ht, dr);
			if (dr_sect_cnt < qsw) { /* calculate min(qsw, dr_sect_cnt) */
				/* ..... */ (void)u16_ht_incr(&pruner->sect_ht, dr, 1);
				int dr_accu_cnt = u16_ht_incr(&pruner->accu_ht, dr, 1);
				if (dr_accu_cnt > ret.max) {
					ret.dr = dr;
					ret.max = dr_accu_cnt;
				}
			}
		}

#ifdef MATH_PRUNING_SECTR_DROP_ENABLE
		/* get more precise estimate, drop early. */
		upbound -= qsw;
		if (ret.max + upbound <= widest) {
			ret.max = 0;
			break;
		}
#endif

#ifdef DEBUG_MATH_SCORE_INSPECT
		if (inspect) {
			printf("\t ");
			u16_ht_print(&pruner->accu_ht);
			printf("\t max: %d, dr: %d\n", ret.max, ret.dr);
		}
#endif
	}

	return ret;
}

struct pq_align_res
math_l2_postlist_widest_match(struct math_l2_postlist *po, float threshold)
{
	struct pq_align_res widest = {0};
	struct math_pruner *pruner = &po->pruner;
	uint64_t candidate = po->candidate;

#ifdef DEBUG_MATH_SCORE_INSPECT
	P_CAST(p, struct math_postlist_item, &candidate);
	int inspect = score_inspect_filter(p->doc_id, po->indices);
#endif

#ifdef MATH_PRUNING_EARLY_DROP_ENABLE
	/* hit width upperbound */
	int hit_width_upp = 0;
	int hit_width_max = 0;
#endif

	/* collect all unique hit nodes */
	int n_save = 0;
	int save_idx[MAX_NODE_IDS];
	u16_ht_reset(&pruner->q_hit_nodes_ht, 0);
	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);

		if (cur == candidate) {
			/* for hit posting lists, save the corresponding nodes. */
			uint32_t pid = po->iter->map[i];
			struct node_set ns = pruner->postlist_nodes[pid];
			for (int j = 0; j < ns.sz; j++) {
				int qid = ns.rid[j];
				/* collect qid */
				if (-1 == u16_ht_lookup(&pruner->q_hit_nodes_ht, qid)) {
					int qid_idx = pruner->nodeID2idx[qid];
					save_idx[n_save++] = qid_idx;
					/* put into set to avoid counting duplicates */
					u16_ht_incr(&pruner->q_hit_nodes_ht, qid, 1);
#ifdef MATH_PRUNING_EARLY_DROP_ENABLE
					struct pruner_node *q_node = pruner->nodes + qid_idx;
					if (hit_width_max < q_node->width)
						hit_width_max = q_node->width;
#endif
				}
			}
#ifdef MATH_PRUNING_EARLY_DROP_ENABLE
			/* increase hit width upperbound */
			hit_width_upp += pruner->postlist_max[pid];
#endif
		}
	}

#ifdef MATH_PRUNING_EARLY_DROP_ENABLE
	/* match tree width <= hit width upperbound <= threshold width */
	/* (not using upp[] here due to possible out-of-array access) */
	uint32_t qw = po->mqs->subpaths.n_lr_paths;
	if (math_expr_score_upperbound(hit_width_upp, qw) <= threshold)
		return widest; /* widest is 0 */
	else if (pruner->upp[hit_width_max] <= threshold)
		return widest; /* widest is 0 */
#endif

	/* calculate score for each hit query node */
	for (int i = 0; i < n_save; i++) {
		/* for each saved hit query node ... */
		int q_node_idx = save_idx[i];
		struct pruner_node *q_node = pruner->nodes + q_node_idx;
		float q_node_upperbound = pruner->upp[q_node->width];

#ifdef MATH_PRUNING_ENABLE /* === Math pruning begin === */
		/* check whether we can drop this node */
		if (q_node_upperbound <= threshold) {
#if defined(DEBUG_MATH_PRUNING) || defined(DEBUG_MATH_SCORE_INSPECT)
#ifdef DEBUG_MATH_SCORE_INSPECT
	//if (inspect)
#endif
	printf("drop node#%d width %u upperbound %f <= threshold %f\n",
		q_node->secttr[0].rnode, q_node->width, q_node_upperbound, threshold);
#endif
			math_pruner_dele_node_safe(pruner, q_node_idx, save_idx, n_save);
			math_pruner_update(pruner);
			math_l2_postlist_sort(po);
			//math_pruner_print(pruner);
			continue; /* so that we can dele other nodes */
		} else if (q_node->width <= widest.width) {
#if defined(DEBUG_MATH_PRUNING) || defined(DEBUG_MATH_SCORE_INSPECT)
#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect)
#endif
	printf("skip node#%d (upperbound <= widest = %d)\n",
		q_node->secttr[0].rnode, widest.width);
#endif
			continue;
		}
#endif                     /* === Math pruning end === */

		/* otherwise, calculate node score */
#ifdef DEBUG_MATH_SCORE_INSPECT
		struct q_node_match qm = calc_q_node_match(po, q_node, widest.width, inspect);
#else
		struct q_node_match qm = calc_q_node_match(po, q_node, widest.width);
#endif
		if (qm.max > widest.width) {
#if defined(DEBUG_MATH_PRUNING) || defined(DEBUG_MATH_SCORE_INSPECT)
#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect)
#endif
	printf("update node#%d widest: %d\n", q_node->secttr[0].rnode, qm.max);
#endif
			widest.width = qm.max;
			widest.qr = q_node->secttr[0].rnode;
			widest.dr = qm.dr;
		}
	}

#ifdef MATH_PRUNING_ENABLE /* === Math pruning begin === */
	if (widest.width && pruner->upp[widest.width] <= threshold) {
#if defined(DEBUG_MATH_PRUNING) || defined(DEBUG_MATH_SCORE_INSPECT)
#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect)
#endif
		printf("item widest upperbound less than threshold, skip.\n");
#endif
		widest.width = 0;
	}
#endif                     /* === Math pruning end === */

#if defined(DEBUG_MATH_PRUNING) || defined(DEBUG_MATH_SCORE_INSPECT)
#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect)
#endif
		printf("\n");
#endif
	return widest;
}
