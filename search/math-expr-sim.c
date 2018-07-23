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

//#define DEBUG_MATH_SCORE_INSPECT

static int
score_inspect_filter(doc_id_t doc_id, struct indices *indices)
{
	size_t url_sz;
	int ret = 0;
	char *url = get_blob_string(indices->url_bi, doc_id, 0, &url_sz);
	char *txt = get_blob_string(indices->txt_bi, doc_id, 1, &url_sz);
//	if (0 == strcmp(url, "Quantum_finance:1") ||
//	    0 == strcmp(url, "Order_statistic:46")) {

	if (doc_id == 96281 || doc_id == 503653) {

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
	hit->score = (uint32_t)100;
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

	score = score * 100000.f;
	hit->score = (uint32_t)(score);
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

	score = score * 100000.f;
	hit->score = (uint32_t)(score);
}

/* joint nodes experiment */
void math_expr_set_score_3(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	struct pq_align_res *ar = factor->align_res;
	uint32_t jo = factor->joint_nodes;
	uint32_t lcs = factor->lcs;
	uint32_t qnn = factor->qry_nodes;
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
	float stj = (float)jo;
	float st = (st0 + st1 + st2 + stj) / (float)qnn;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));
	(void)lcs;

	score = score * 100000.f;
	hit->score = (uint32_t)(score);
}

void
math_expr_set_score(struct math_expr_sim_factors* factor,
                    struct math_expr_score_res* hit)
{
	//math_expr_set_score_0(factor, hit);

	//math_expr_set_score_7(factor, hit);
	//math_expr_set_score_10(factor, hit);
	
#ifdef MATH_SLOW_SEARCH
	math_expr_set_score_2(factor, hit);
	//math_expr_set_score_3(factor, hit);
#else
	math_expr_set_score_1(factor, hit);
#endif
}

static mnc_score_t
prefix_symbolset_similarity(uint64_t cur_min, struct postmerge* pm,
                            struct math_postlist_item *items[],
                            struct pq_align_res *align_res, uint32_t k)
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
				ql = mepa->ele->dup[j]->path_id;

				for (uint32_t m = 0; m < k; m++) {
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
				uint32_t qn = mepa->ele->dup[j]->node_id;
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
//	uint32_t                    i, j, k;
//	math_posting_t              posting;
//	uint32_t                    pathinfo_pos;
//	struct math_posting_item   *po_item;
//	struct math_pathinfo_pack  *pathinfo_pack;
//	struct math_pathinfo       *pathinfo;
//	struct subpath_ele         *subpath_ele;
//	int ret = 0;
//
//	enum symbol_id querystr[MAX_LEAVES] = {0};
//	enum symbol_id candistr[MAX_LEAVES] = {0};
//
//	for (i = 0; i < pm->n_postings; i++) {
//		posting = pm->postings[i];
//		subpath_ele = NULL;//math_posting_get_ele(posting);
//
//		pathinfo_pos = po_item->pathinfo_pos;
//		pathinfo_pack = math_posting_pathinfo(posting, pathinfo_pos);
//
//		if (NULL == pathinfo_pack || NULL == subpath_ele) {
//			return 0;
//		}
//
//		for (j = 0; j < pathinfo_pack->n_paths; j++) {
//			pathinfo = pathinfo_pack->pathinfo + j;
//			assert(pathinfo->path_id <= MAX_LEAVES);
//			candistr[pathinfo->path_id - 1] = pathinfo->lf_symb;
//
//			for (k = 0; k <= subpath_ele->dup_cnt; k++) {
//				enum symbol_id qs = subpath_ele->dup[k]->lf_symbol_id;
//				uint32_t qn = subpath_ele->dup[k]->node_id;
//				querystr[qn - 1] = qs;
//			}
//		}
//	}
//
//	if (pm->n_postings != 0) {
//		ret = substring_filter(querystr, candistr);
////		if (0) {
////		//if (8325 == po_item->exp_id) {
////			printf("Query: ");
////			for (i = 0; i < MAX_LEAVES; i++) {
////				if (querystr[i] == 0)
////					break;
////				printf("%s ", trans_symbol(querystr[i]));
////			} printf("\n");
////			printf("expr#%d, Candi: ", po_item->exp_id);
////			for (i = 0; i < MAX_LEAVES; i++) {
////				printf("%s ", trans_symbol(candistr[i]));
////			} printf("\n");
////			printf("Return: %d\n", ret);
////		}
//	}
//
//	return ret;
}

struct math_expr_score_res
math_expr_score_on_merge(struct postmerge* pm,
                         uint32_t level, uint32_t n_qry_lr_paths)
{
	struct math_expr_score_res  ret = {0};
	return ret;

//	uint32_t                    i, j, k;
//	math_posting_t              posting;
//	uint32_t                    pathinfo_pos;
//	struct math_posting_item   *po_item;
//	struct math_pathinfo_pack  *pathinfo_pack;
//	struct math_pathinfo       *pathinfo;
//	struct subpath_ele         *subpath_ele;
//	bool                        skipped = 0;
//
//	/* reset mnc for scoring new document */
//	uint32_t slot;
//	struct mnc_ref mnc_ref;
//	mnc_reset_docs();
//
//	for (i = 0; i < pm->n_postings; i++) {
//		/* for each merged posting item from posting lists */
//		posting = pm->postings[i];
//		subpath_ele = NULL;//math_posting_get_ele(posting);
//
//		/* get pathinfo position of corresponding merged item */
//		pathinfo_pos = po_item->pathinfo_pos;
//
//		/* use pathinfo position to get pathinfo packet */
//		pathinfo_pack = math_posting_pathinfo(posting, pathinfo_pos);
//
//		if (NULL == pathinfo_pack || NULL == subpath_ele) {
//			/* unexpected read error, e.g. file is corrupted */
//#ifdef DEBUG_MATH_EXPR_SEARCH
//			fprintf(stderr, "pathinfo_pack or subpath_ele is NULL.\n");
//#endif
//			skipped = 1;
//			break;
//		}
//
//		if (n_qry_lr_paths > pathinfo_pack->n_lr_paths) {
//			/* impossible to match, skip this math expression */
//#ifdef DEBUG_MATH_EXPR_SEARCH
//			printf("query leaf-root paths (%u) is greater than "
//			       "document leaf-root paths (%u), skip this expression."
//			       "\n", n_qry_lr_paths, pathinfo_pack->n_lr_paths);
//#endif
//			skipped = 1;
//			break;
//		}
//
//		for (j = 0; j < pathinfo_pack->n_paths; j++) {
//			/* for each pathinfo from this pathinfo packet */
//			pathinfo = pathinfo_pack->pathinfo + j;
//
//			/* preparing to score corresponding document subpaths */
//			mnc_ref.sym = pathinfo->lf_symb;
//			slot = mnc_map_slot(mnc_ref);
//
//			for (k = 0; k <= subpath_ele->dup_cnt; k++) {
//				/*
//				 * add this document subpath for scoring.
//				 * (path_id [1, MAX_LEAVES] is mapped to [0, 63])
//				 */
//				mnc_doc_add_rele(slot, pathinfo->path_id - 1,
//				                 subpath_ele->dup[k]->path_id - 1);
//			}
//		}
//	}
//
//#ifdef DEBUG_MATH_EXPR_SEARCH
//	printf("query leaf-root paths: %u\n", n_qry_lr_paths);
//	printf("document leaf-root paths: %u\n", pathinfo_pack->n_lr_paths);
//	printf("posting merge level: %u\n", level);
//#endif
//
//	/* finally calculate expression similarity score */
//	if (!skipped && pm->n_postings != 0) {
//		ret.score = mnc_score(true);
//		ret.doc_id = po_item->doc_id;
//		ret.exp_id = po_item->exp_id;
//	}
//
//	return ret;
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
				ql = mepa->ele->dup[j]->path_id;
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
#ifdef DEBUG_MATH_SCORE_INSPECT
//					if (inspect) {
//						printf("\t\t doc prefix path [%u ~ %u, %s]\n", dr, dl,
//						       trans_symbol(item->lf_symb[k]));
//						printf("\t\t hit returns 0x%lu, n_dirty = %u \n", res, pq->n_dirty);
//						//pq_print(*pq, 26);
//						printf("\n");
//					}
#else
					(void)res;
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
	uint32_t r_cnt = pq_align(pq, align_res, MAX_MTREE);

#ifdef DEBUG_MATH_SCORE_INSPECT
	if (inspect)
		pq_print_dirty_array(pq);
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

		/* set postional information */
		for (int i = 0; i < MAX_MTREE; i++) {
			if (align_res[i].width) {
				ret.qmask[i] = align_res[i].qmask;
				ret.dmask[i] = align_res[i].dmask;
			}
		}
		
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
