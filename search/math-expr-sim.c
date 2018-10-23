#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "common/common.h"
#include "tex-parser/head.h"
#include "indexer/config.h"
#include "indexer/index.h"
//#include "math-index/math-index.h"
#include "postlist/math-postlist.h"

#include "config.h"
#include "search-utils.h"
#include "math-expr-search.h"
#include "math-prefix-qry.h"
#include "math-expr-sim.h"

// #define DEBUG_MATH_SCORE_INSPECT

static int
score_inspect_filter(doc_id_t doc_id, struct indices *indices)
{
	size_t url_sz;
	int ret = 0;
	char *url = get_blob_string(indices->url_bi, doc_id, 0, &url_sz);
	char *txt = get_blob_string(indices->txt_bi, doc_id, 1, &url_sz);
//	if (0 == strcmp(url, "Scattering_parameters:115") ||
//	    0 == strcmp(url, "Port_(circuit_theory):0")) {

	if (doc_id == 230891 || doc_id == 568493) {

	// if (0 == strcmp(url, "Dimension_theory_(algebra):45")) {

		printf("%s: doc %u, url: %s\n", __func__, doc_id, url);
		// printf("%s \n", txt);
		ret = 1;
	}

	free(url);
	free(txt);
	return ret;
}

void math_expr_set_score_0(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	hit->score = 1.f;
}

/* CIKM-2018 run-1 */
void math_expr_set_score_1(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	struct pq_align_res *ar = factor->align_res;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float alpha = 0.05f;
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));
	float st0 = (float)ar[0].width /(float)(qn);
	float st = st0;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));

	hit->score = score;
}

/* CIKM-2018 run-2 */
void math_expr_set_score_2(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	struct pq_align_res *ar = factor->align_res;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float alpha = 0.05f;
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));
	float st0 = (float)ar[0].width / (float)(qn);
	float st1 = (float)ar[1].width / (float)(qn);
	float st2 = (float)ar[2].width / (float)(qn);
	float st = 0.65f * st0 + 0.3 * st1 + 0.05f * st2;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));

	hit->score = score;
}

#ifdef MATH_SLOW_SEARCH
/* experimental scoring function  */
void math_expr_set_score_3(struct math_expr_sim_factors* factor,
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
	const float alpha = 0.4f;
	const float beta[5] = {
		0.70f,
		0.20f,
		0.10f,
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
#endif

void math_expr_set_score_fast(struct math_expr_sim_factors* factor,
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

#ifdef PQ_CELL_NUMERIC_WEIGHT
/* CIKM-2018 run-2 with path length weight */
void math_expr_set_score_4(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	uint32_t qnn = factor->qry_nodes;
	struct pq_align_res *ar = factor->align_res;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float alpha = 0.05f;
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));
	float st0 = (float)ar[0].width / (float)(qnn);
	float st1 = (float)ar[1].width / (float)(qnn);
	float st2 = (float)ar[2].width / (float)(qnn);
	float st = 0.65f * st0 + 0.3 * st1 + 0.05f * st2;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));

	hit->score = score;
}
#endif

void
math_expr_set_score(struct math_expr_sim_factors* factor,
                    struct math_expr_score_res* hit)
{
	//math_expr_set_score_0(factor, hit);

	//math_expr_set_score_7(factor, hit);
	//math_expr_set_score_10(factor, hit);

#ifdef MATH_SLOW_SEARCH
	// math_expr_set_score_2(factor, hit);
	math_expr_set_score_3(factor, hit);
	// math_expr_set_score_4(factor, hit);
#else
	math_expr_set_score_fast(factor, hit);
#endif
}

static mnc_score_t
prefix_symbolset_similarity(uint64_t cur_min, struct postmerge* pm,
                            struct math_postlist_item *items[],
                            struct pq_align_res *align_res, uint32_t n_mtrees)
{
	/* reset mnc for scoring new document */
	mnc_reset_docs();

	for (uint32_t i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			PTR_CAST(mepa, struct math_extra_posting_arg, pm->posting_args[i]);
			struct math_postlist_item *item = items[i];

			for (uint32_t j = 0; j <= mepa->ele->dup_cnt; j++) {
				uint32_t qr, ql;
				qr = mepa->ele->rid[j];
				ql = mepa->ele->dup[j]->path_id; /* use path_id for mnc_score */
				/* (so that the ql is the index of query array in MNC scoring */

				for (uint32_t m = 0; m < n_mtrees; m++) {
					if (qr == align_res[m].qr) {
						for (uint32_t k = 0; k < item->n_paths; k++) {
							uint32_t dr, dl;
							dr = item->subr_id[k];
							dl = item->leaf_id[k];
							if (dr == align_res[m].dr) {
								uint32_t slot;
								struct mnc_ref mnc_ref;
								mnc_ref.sym = item->lf_symb[k];
								slot = mnc_map_slot(mnc_ref);
								mnc_doc_add_rele(slot, dl - 1, ql - 1);

#ifdef DEBUG_MATH_SCORE_INSPECT
//if (po_item->doc_id == 68557 || po_item->doc_id == 97423) {
//enum symbol_id qs = subpath_ele->dup[j]->lf_symbol_id;
//printf("prefix MNC:  qpath#%u `%s' --- dpath#%u `%s' "
//	   "@slot%u\n", ql, trans_symbol(qs), dl, trans_symbol(p->lf_symb),
//	   slot);
//}
#endif
							}
						}
					}
				}
			}
		} /* end if */
	} /* end for */

	return mnc_score(false);
}

int string_longest_common_substring(enum symbol_id *str1, enum symbol_id *str2)
{
	int (*DP)[MAX_LEAVES] = calloc(MAX_LEAVES, MAX_LEAVES * sizeof(int));
	int lcs = 0;
	int i, j;
	for (i = 0; i < MAX_LEAVES; i++) {
		for (j = 0; j < MAX_LEAVES; j++) {
			if (i == 0 || j == 0) {
				DP[i][j] = 0;
			} else if (str1[i-1] == str2[j-1] &&
					   str1[i-1] != 0) {
				DP[i][j] = DP[i-1][j-1] + 1;
				if (DP[i][j] > lcs)
					lcs = DP[i][j];
			} else {
				DP[i][j] = 0;
			}
		}
	}
	free(DP);

	return lcs;
}

static int
substring_filter(enum symbol_id *str1, enum symbol_id *str2)
{
	int i;
	for (i = 0; i < MAX_LEAVES; i++) {
		if (str1[i] == 0)
			return 1;
		if (str1[i] != str2[i])
			return 0;
	}

	return 1;
}

static int
prefix_symbolseq_similarity(uint64_t cur_min, struct postmerge* pm)
{
	int i, j, k;

	enum symbol_id querystr[MAX_LEAVES] = {0};
	enum symbol_id candistr[MAX_LEAVES] = {0};

	for (i = 0; i < pm->n_postings; i++) {
		PTR_CAST(item, struct math_postlist_item, POSTMERGE_CUR(pm, i));
		PTR_CAST(mepa, struct math_extra_posting_arg, pm->posting_args[i]);

		if (pm->curIDs[i] == cur_min) {
			for (j = 0; j <= mepa->ele->dup_cnt; j++) {
				uint32_t qn = mepa->ele->dup[j]->leaf_id; /* use leaf_id for order ID */
				enum symbol_id qs = mepa->ele->dup[j]->lf_symbol_id;
				querystr[qn - 1] = qs;

				for (k = 0; k < item->n_paths; k++) {
					candistr[item->leaf_id[k] - 1] = item->lf_symb[k];
				}
			}
		} /* end if */
	} /* end for */

	return string_longest_common_substring(querystr, candistr);
////	for (i = 0; i < MAX_LEAVES; i++) {
////		printf("%s ", trans_symbol(querystr[i]));
////	} printf("\n");
////	for (i = 0; i < MAX_LEAVES; i++) {
////		printf("%s ", trans_symbol(candistr[i]));
////	} printf("\n");
////	printf("lcs = %u\n", lcs);
}

static int
symbolseq_similarity(struct postmerge* pm)
{
	return 0;
}

struct math_expr_score_res
math_expr_score_on_merge(struct postmerge* pm,
                         uint32_t level, uint32_t n_qry_lr_paths)
{
	struct math_expr_score_res  ret = {0};
	return ret;
}

struct math_expr_score_res
math_expr_prefix_score_on_merge(
	uint64_t cur_min, struct postmerge *pm,
	struct math_extra_score_arg *mesa, struct indices *indices
)
{
	struct math_expr_score_res     ret = {0};
	struct math_postlist_item *po_item = NULL;

	struct math_prefix_qry    *pq = &mesa->pq;
	mnc_score_t               symbol_sim = 0;
	int                       lcs = 0;
	struct math_postlist_item *items[pm->n_postings];

#ifdef DEBUG_MATH_SCORE_INSPECT
	int inspect = 0;
#endif

	for (uint32_t i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			PTR_CAST(mepa, struct math_extra_posting_arg, pm->posting_args[i]);
			items[i] = (struct math_postlist_item*)POSTMERGE_CUR(pm, i);
			struct math_postlist_item *item = items[i];

			po_item = item;

#ifdef DEBUG_MATH_SCORE_INSPECT
		inspect = score_inspect_filter(po_item->doc_id, indices);
#endif
			for (uint32_t j = 0; j <= mepa->ele->dup_cnt; j++) {
				uint32_t qr, ql;
				qr = mepa->ele->rid[j];
				ql = mepa->ele->dup[j]->leaf_id; /* use leaf_id for aligning */
				/* (to generate correct leaf mask to be highlighted in tree) */
#ifdef DEBUG_MATH_SCORE_INSPECT
//				if (inspect) {
//					printf("\t qry prefix path [%u ~ %u, %s] hits: \n", qr, ql,
//					       trans_symbol(mepa->ele->dup[j]->lf_symbol_id));
//				}
#endif
				for (uint32_t k = 0; k < item->n_paths; k++) {
					uint32_t dr, dl;
					dr = item->subr_id[k];
					dl = item->leaf_id[k];

					uint64_t res = 0;
					res = pq_hit(pq, qr, ql, dr, dl);
					(void)res;
#ifdef DEBUG_MATH_SCORE_INSPECT
//					if (inspect) {
//						printf("\t\t doc prefix path [%u ~ %u, %s]\n", dr, dl,
//						       trans_symbol(item->lf_symb[k]));
//						printf("\t\t hit returns 0x%lu, n_dirty = %u \n", res, pq->n_dirty);
//						//pq_print(*pq, 26);
//						printf("\n");
//					}
#endif
				}
			}
#ifdef DEBUG_MATH_SCORE_INSPECT
//			if (inspect) {
//				printf("}\n");
//			}
#endif
		}
	}

	/* sub-structure align */
	struct pq_align_res align_res[MAX_MTREE] = {0};
	uint32_t r_cnt = pq_align(pq, align_res, NULL);

#ifdef DEBUG_MATH_SCORE_INSPECT
//	if (inspect)
//		pq_print_dirty_array(pq);
#endif
	pq_reset(pq);

	/* symbol set similarity */
	symbol_sim = prefix_symbolset_similarity(cur_min, pm, items, align_res, MAX_MTREE);

	/* symbol sequence similarity */
	//lcs = prefix_symbolseq_similarity(cur_min, pm);
	(void)lcs;

	if (po_item) {
		uint32_t dn = (po_item->n_lr_paths) ? po_item->n_lr_paths : MAX_MATH_PATHS;
		struct math_expr_sim_factors factors = {
			symbol_sim, 0 /* search depth */, mesa->n_qry_lr_paths, dn,
			align_res, MAX_MTREE, r_cnt, lcs, mesa->n_qry_max_node
		};
		ret.doc_id = po_item->doc_id;
		ret.exp_id = po_item->exp_id;

#ifdef MATH_SLOW_SEARCH
		/* set postional information */
		for (int i = 0; i < MAX_MTREE; i++) {
			if (align_res[i].width) {
				ret.qmask[i] = align_res[i].qmask;
				ret.dmask[i] = align_res[i].dmask;
			}
		}
#endif

#ifdef DEBUG_MATH_SCORE_INSPECT
		if (inspect) {
			math_expr_set_score(&factors, &ret);
			printf("doc#%u, exp#%u, final score: %u\n",
			       ret.doc_id, ret.exp_id, ret.score);
		}
#else
		math_expr_set_score(&factors, &ret);
#endif
	}

	return ret;
}

void math_l2_postlist_print_cur(struct math_l2_postlist *po)
{
	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		uint32_t docID = (uint32_t)(cur >> 32);
		uint32_t expID = (uint32_t)(cur >> 0);

		uint32_t orig = po->iter->map[i];
		printf("[%s] [%u] -> [%u]: %u,%u \n", po->type[orig], orig, i,
				docID, expID);
	}
}

#ifdef DEBUG_MATH_SCORE_INSPECT
static int debug = 0;
#endif

uint32_t
math_l2_postlist_cur_struct_sim(struct math_l2_postlist *po,
	struct pq_align_res *ar, uint32_t *r_cnt)
{
	struct math_postlist_item item = {0};
	uint32_t n_doc_lr_paths = 0;
	struct math_prefix_qry *pq = &po->mqs->pq;

	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		uint32_t orig = po->iter->map[i];
		struct subpath_ele *ele = po->ele[orig];

		if (cur != UINT64_MAX && cur == po->iter->min) {
			postmerger_iter_call(&po->pm, po->iter, read, i, &item, sizeof(item));
			n_doc_lr_paths = item.n_lr_paths;

			for (uint32_t j = 0; j <= ele->dup_cnt; j++) {
				uint32_t qr = ele->rid[j];
				uint32_t ql = ele->dup[j]->leaf_id; /* use leaf_id for alignment */
				for (uint32_t k = 0; k < item.n_paths; k++) {
					uint32_t dr = item.subr_id[k];
					uint32_t dl = item.leaf_id[k];


#ifdef DEBUG_MATH_SCORE_INSPECT
//	if (debug)
//		printf("qr~ql %u~%u, dr~dl %u~%u. \n", qr, ql, dr, dl);
#endif
					uint64_t res = 0;
#ifdef PQ_CELL_NUMERIC_WEIGHT
					uint32_t w = ele->prefix_len;
					res = pq_hit_numeric(pq, qr, ql, dr, dl, w);
#else
					res = pq_hit(pq, qr, ql, dr, dl);
#endif
					(void)res;
				}
			}
		}
	}

	*r_cnt = pq_align(pq, ar, po->mqs->visibimap);
	pq_reset(pq);

	return n_doc_lr_paths;
}

mnc_score_t
math_l2_postlist_cur_symbol_sim(struct math_l2_postlist *po, struct pq_align_res *ar)
{
	struct math_postlist_item item = {0};
	mnc_reset_docs();

	for (int i = 0; i < po->iter->size; i++) {
		uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, i);
		uint32_t orig = po->iter->map[i];
		struct subpath_ele *ele = po->ele[orig];

		if (cur != UINT64_MAX && cur == po->iter->min) {
			postmerger_iter_call(&po->pm, po->iter, read, i, &item, sizeof(item));

			for (uint32_t j = 0; j <= ele->dup_cnt; j++) {
				uint32_t qr = ele->rid[j];
				uint32_t ql = ele->dup[j]->path_id; /* use path_id for mnc_score */
				for (uint32_t m = 0; m < MAX_MTREE; m++) {
					if (qr == ar[m].qr) {
						for (uint32_t k = 0; k < item.n_paths; k++) {
							uint32_t dr = item.subr_id[k];
							uint32_t dl = item.leaf_id[k];

							if (dr == ar[m].dr) {
								uint32_t slot;
								struct mnc_ref mnc_ref;
								mnc_ref.sym = item.lf_symb[k];
								slot = mnc_map_slot(mnc_ref);
								mnc_doc_add_rele(slot, dl - 1, ql - 1);
							}
						}
					}
				}
			}
		} /* end if */
	} /* end for */

	return mnc_score(false);
}

struct math_expr_score_res
math_l2_postlist_cur_score(struct math_l2_postlist *po)
{
	struct math_expr_score_res ret = {0};

	uint32_t r_cnt, n_doc_lr_paths, lcs = 0;
	struct pq_align_res align_res[MAX_MTREE] = {0};

	/* get docID and exprID */
	uint64_t cur = po->iter->min;
	P_CAST(item, struct math_postlist_item, &cur);
	ret.doc_id = item->doc_id;
	ret.exp_id = item->exp_id;

#ifdef DEBUG_MATH_SCORE_INSPECT
	debug = 0; if (score_inspect_filter(ret.doc_id, po->indices)) debug = 1;
//	if (debug)
//		math_l2_postlist_print_cur(po);
#endif

	/* structure scores */
	n_doc_lr_paths = math_l2_postlist_cur_struct_sim(po, align_res, &r_cnt);


#ifdef DEBUG_MATH_SCORE_INSPECT
	if (debug) {
		printf("doc#%u, exp#%u\n", ret.doc_id, ret.exp_id);
		for (int i = 0; i < MAX_MTREE; i++) {
			printf("tree#%d: width=%u, qr=%u, dr=%u, qmask=0x%lx, dmask=0x%lx\n",
				i, align_res[i].width, align_res[i].qr, align_res[i].dr,
				align_res[i].qmask, align_res[i].dmask);
			for (uint32_t bit = 0; bit < MAX_LEAVES; bit++) {
				if ((0x1L << bit) & align_res[i].qmask)
					printf("q%u ", bit + 1);
				if ((0x1L << bit) & align_res[i].dmask)
					printf("d%u ", bit + 1);
			}
			printf("\n");
		}
		printf("align r_cnt = %u\n", r_cnt);
	}
#endif

	/* symbolic scores */
	mnc_score_t sym_sim = math_l2_postlist_cur_symbol_sim(po, align_res);

	/* get doc/qry number of leaf-root paths */
	uint32_t qn = po->mqs->subpaths.n_lr_paths;
	uint32_t dn = (n_doc_lr_paths) ? n_doc_lr_paths : MAX_MATH_PATHS;

	/* similarity factors */
	struct math_expr_sim_factors factors = {
		sym_sim, 0 /* search depth, not used */, qn, dn,
		align_res, MAX_MTREE, r_cnt, lcs, po->mqs->n_qry_nodes
	};

	/* calculate similarity score */
	math_expr_set_score(&factors, &ret);

#ifdef MATH_SLOW_SEARCH
	/* set postional information */
	for (int i = 0; i < MAX_MTREE; i++) {
		if (align_res[i].width) {
			ret.qmask[i] = align_res[i].qmask;
			ret.dmask[i] = align_res[i].dmask;
		}
	}
#endif

	return ret;
}

////////////////////  pruning functions ////////////////////////////////

static float upperbound(int width, struct math_l2_postlist *po)
{
	const float theta = 0.05f;
	float st = (float)width * po->inv_qw;
	float fmeasure = st / (st + 1.f);
	return fmeasure * ((1.f - theta) + theta * 1.443f);
}

static float lowerbound(int width, struct math_l2_postlist *po)
{
	const float theta = 0.05f;
	float st = (float)width * po->inv_qw;
	float fmeasure = (st * 0.5f) / (st + 0.5f);
	return fmeasure * ((1.f - theta) + theta * 0.240f);
}

uint32_t
math_l2_postlist_coarse_score(struct math_l2_postlist *po,
                              struct pq_align_res *widest)
{
	float min_score = 0.f;
	struct math_pruner *pruner = &po->pruner;
	uint32_t n_doc_lr_paths = 0;

	if (priority_Q_full(po->rk_res))
		min_score = priority_Q_min_score(po->rk_res);

	/* for each query node in sorted order */
	for (int i = 0; i < pruner->n_nodes; i++) {
		struct pruner_node *q_node = pruner->nodes + i;
		float q_node_upperbound = upperbound(q_node->width, po);

		/* check whether we can prune this node */
		if (q_node_upperbound <= min_score) {
			for (int j = 0; j < q_node->n; j++) {
				int p = q_node->postlist_id[j];
				pruner->postlist_ref[p] -= 1;
			}
			math_pruner_dele_node(pruner, i);
			i -= 1;
			continue;

		} else if (q_node->width < widest->width &&
		    q_node_upperbound <= lowerbound(widest->width, po)) {
			break;
		}

		/* calculate query node max match */
		int q_node_dr, q_node_match = 0;
		int accu_vec[MAX_NODE_IDS] = {0};
		for (int j = 0; j < q_node->n; j++) {
			/* read document posting list item */
			struct math_postlist_item item;
			const size_t sz = sizeof(item);
			int p = q_node->postlist_id[j];
			uint64_t cur = postmerger_iter_call(&po->pm, po->iter, cur, p);
			if (cur == UINT64_MAX || cur != po->iter->min) continue;
			postmerger_iter_call(&po->pm, po->iter, read, p, &item, sz);
			n_doc_lr_paths = item.n_lr_paths; /* save doc OPT width */

			/* match counter vector calculation */
			int qsw = q_node->secttr[j].width;
			int sect_vec[MAX_NODE_IDS] = {0};
			for (int k = 0; k < item.n_paths; k++) {
				int dr = item.subr_id[k];
				if (sect_vec[dr] < qsw) {
					sect_vec[dr] ++;
					accu_vec[dr] ++;
					if (accu_vec[dr] > q_node_match) {
						q_node_dr = dr;
						q_node_match = accu_vec[dr];
					}
				}
			}
		}

		/* update widest match */
		if (q_node_match > widest->width) {
			widest->width = q_node_match;
			widest->qr = q_node->secttr[0].rnode;
			widest->dr = q_node_dr;
		}
	}

	return n_doc_lr_paths;
}

struct math_expr_score_res
math_l2_postlist_precise_score(struct math_l2_postlist *po,
                               struct pq_align_res *widest,
                               uint32_t n_doc_lr_paths)
{
	struct math_expr_score_res expr;
	/* get docID and exprID */
	uint64_t cur = po->iter->min;
	P_CAST(p, struct math_postlist_item, &cur);
	expr.doc_id = p->doc_id;
	expr.exp_id = p->exp_id;
	/* get overall score */
	mnc_score_t sym_sim = math_l2_postlist_cur_symbol_sim(po, widest);
	uint32_t qn = po->mqs->subpaths.n_lr_paths;
	uint32_t dn = (n_doc_lr_paths) ? n_doc_lr_paths : MAX_MATH_PATHS;
	struct math_expr_sim_factors factors = {
		sym_sim, 0 /* search depth*/, qn, dn, widest, 1 /* k */,
		0 /* r_cnt */, 0 /* lcs */, 0 /* n_qry_nodes */
	};
	math_expr_set_score(&factors, &expr);
	return expr;
}
