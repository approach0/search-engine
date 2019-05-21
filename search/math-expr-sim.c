#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "common/common.h"
#include "tex-parser/head.h"
#include "indexer/config.h"
#include "indexer/index.h"
#include "postlist/math-postlist.h"
#include "hashtable/u16-ht.h"

#include "config.h"
#include "search-utils.h"
#include "math-prefix-qry.h"
#include "math-expr-sim.h"
#include "math-search.h"

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

//	if (NULL != strstr(url, "/480364/" /* bad */) ||
//	    NULL != strstr(url, "/879729/" /* good but missed */)) {

//	if (NULL != strstr(url, "/179098/")) {

	if (doc_id == 1 || doc_id == 2) {

//	if (doc_id == 150175) {

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

	// math_expr_set_score__multi_tree(factor, hit);
	math_expr_set_score__opd_only(factor, hit);
}

void math_l2_postlist_print_cur(struct math_l2_postlist *po)
{
	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(po->iter, cur, i);
		uint32_t docID = (uint32_t)(cur >> 32);
		uint32_t expID = (uint32_t)(cur >> 0);

		uint32_t orig = po->iter->map[i];
		printf("po#%u under iter[%u]: doc#%u,exp#%u \n", orig, i, docID, expID);
	}
}

/* symbolset similarity calculation, single-tree only. */
static struct mnc_match_t
#ifdef DEBUG_MATH_SCORE_INSPECT
math_l2_postlist_cur_match(struct math_l2_postlist *po,
	struct pq_align_res *ar, int inspect)
#else
math_l2_postlist_cur_match(struct math_l2_postlist *po,
	struct pq_align_res *ar)
#endif
{
	struct mnc_match_t match;
	struct math_postlist_gener_item item;
	struct math_postlist_item *_item;
	mnc_ptr_t mnc = po->mnc;
	mnc_reset_docs(mnc);

	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(po->iter, cur, i);

		if (cur == po->candidate) {
			uint32_t orig = po->iter->map[i];
			struct subpath_ele *ele = po->ele[orig];
			enum math_posting_type pt = po->path_type[orig];

			for (uint32_t j = 0; j <= ele->dup_cnt; j++) {
				uint32_t qr = ele->rid[j];
				uint32_t ql = ele->dup[j]->path_id; /* use path_id for mnc */
				if (qr == ar->qr) {
					postmerger_iter_call(po->iter, read, i,
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
								mnc_doc_add_rele(mnc, ql - 1, dl - 1, ref);
							} else {
								uint64_t dls = item.wild_leaves[k];
								mnc_doc_add_reles(mnc, ql - 1, dls, ref);
							}
						}
					}
				}
			} /* end for */
		} /* end if */
	} /* end for */
#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect) {
		match = mnc_match_debug(mnc);
	} else
#endif

	match = mnc_match(mnc);
	return match;
}

////////////////////  pruning functions ////////////////////////////////
void math_l2_cur_print(struct math_l2_postlist *po)
{
	float threshold = *po->theta;
	uint64_t candidate = po->candidate;
	struct math_pruner *pruner = &po->pruner;
	printf("doc#%lu, pivot = %d/%u, threshold = %.3f.\n",
		candidate >> 32, pruner->postlist_pivot, po->iter->size, threshold);
	for (int i = 0; i < po->iter->size; i++) {
		uint32_t pid = po->iter->map[i];
		uint64_t cur = postmerger_iter_call(po->iter, cur, i);
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
			printf("pass: iter[%u] ", i);
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
	struct mnc_match_t match;
	uint32_t r_cnt = 0;

	/* get docID and exprID */
	uint64_t cur = po->candidate;
	P_CAST(p, struct math_postlist_item, &cur);
	expr.doc_id = p->doc_id;
	expr.exp_id = p->exp_id;

	/* get symbolic score */
#ifdef DEBUG_MATH_SCORE_INSPECT
	int inspect = score_inspect_filter(p->doc_id, po->indices);
	match = math_l2_postlist_cur_match(po, widest, inspect);
#else
	match = math_l2_postlist_cur_match(po, widest);
#endif

	/* get precise structure match width */
	widest->width = __builtin_popcountll(match.qry_paths);
#ifdef HIGHLIGHT_MATH_ALIGNMENT
	widest->qmask = match.qry_paths;
	widest->dmask = match.doc_paths;
#endif

	/* get single tree overall score */
	const uint32_t k = 1;
	uint32_t qn = po->mqs->subpaths.n_lr_paths;
	struct math_expr_sim_factors factors = {
		match.score, 0 /* search depth*/, qn, dn, widest, k,
		r_cnt, 0 /* lcs */, po->mqs->n_qry_nodes};
	math_expr_set_score(&factors, &expr);

#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect) {
		/* print posting list hits */
		math_l2_cur_print(po);

		math_expr_sim_factors_print(&factors);
#ifdef HIGHLIGHT_MATH_ALIGNMENT
		printf("alignment: qry: 0x%lx, doc: 0x%lx.\n",
		       widest->qmask, widest->dmask);
#endif
		printf("doc#%u, exp#%u, final expr score: %f\n",
		       expr.doc_id, expr.exp_id, expr.score);
		printf("\n");
	}
#endif

	return expr;
}
