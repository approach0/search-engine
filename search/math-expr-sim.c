#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "tex-parser/head.h"
#include "indexer/config.h"
#include "indexer/index.h"
#include "math-index/math-index.h"

#include "search-utils.h"
#include "config.h"
#include "math-expr-search.h"

//#define DEBUG_MATH_SCORE_INSPECT

void math_expr_set_score(struct math_expr_sim_factors*, struct math_expr_score_res*); 

static int
score_inspect_filter(struct math_expr_score_res hit, struct indices *indices)
{
	size_t url_sz;
	char *url = get_blob_string(indices->url_bi, hit.doc_id, 0, &url_sz);
	char *txt = get_blob_string(indices->txt_bi, hit.doc_id, 1, &url_sz);
	//if (0 == strcmp(url, "Correlation_and_dependence:1")) {
	if (0 == strcmp(url, "Models_of_DNA_evolution:80") ||
	    0 == strcmp(url, "Conjectural_variation:15")) {
	//if (hit.doc_id == 227659 || hit.doc_id == 274896) {

		printf("%s: doc %u, expr %u, url: %s\n", __func__,
		        hit.doc_id, hit.exp_id, url);
		// printf("%s \n", txt);
		return 1;
	}

	free(url);
	free(txt);

	return 0;
}


void math_expr_set_score_7(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	uint32_t *t = factor->topk_cnt;//, jo = factor->joint_nodes;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float alpha = 0.05f;
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));
	float st0 = fmaxf(0, (float)t[0]/(float)(qn) - 0.1f);
	float st = st0;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));
	score = score * 100000.f;
	hit->score = (uint32_t)(score);
}

void
math_expr_set_score_10(struct math_expr_sim_factors* factor,
                    struct math_expr_score_res* hit)
{
	uint32_t *t = factor->topk_cnt;//, jo = factor->joint_nodes;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float alpha = 0.05f;
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));
	float st0 = fmaxf(0, (float)t[0]/(float)(qn) - 0.1f);
	float st1 = (float)t[1]/(float)(qn);
	float st2 = (float)t[2]/(float)(qn);
	float st = st0 + st1 + st2 / 10.f;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));
	score = score * 100000.f;
	hit->score = (uint32_t)(score);
}

/* CIKM-2018 run-1 */
void math_expr_set_score_1(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	uint32_t *t = factor->topk_cnt;//, jo = factor->joint_nodes;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float alpha = 0.05f;
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));
	float st0 = (float)t[0]/(float)(qn);
	float st = st0;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));
#ifdef DEBUG_MATH_SCORE_INSPECT
	printf("t = %u, %u, %u, len=%u \n", t[0],t[1],t[2], qn);
	printf("st = %f, %f, %f \n", st0, st1, st2);
	printf("fmeasure = %f, score = %f \n", fmeasure, score);
#endif
	score = score * 100000.f;
	hit->score = (uint32_t)(score);
}

/* CIKM-2018 run-2 */
void math_expr_set_score_2(struct math_expr_sim_factors* factor,
                           struct math_expr_score_res* hit)
{
	uint32_t *t = factor->topk_cnt;//, jo = factor->joint_nodes;
	uint32_t qn = factor->qry_lr_paths;
	uint32_t dn = factor->doc_lr_paths;
	uint32_t nsim = (factor->mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                (qn * MNC_MARK_FULL_SCORE);
	float alpha = 0.05f;
	float sy0 = (float)nsim / MAX_MATH_EXPR_SIM_SCALE;
	float sy = 1.f / (1.f + powf(1.f - (float)(sy0), 2));
	float st0 = (float)t[0]/(float)(qn);
	float st1 = (float)t[1]/(float)(qn);
	float st2 = (float)t[2]/(float)(qn);
	float st = 0.65f * st0 + 0.3 * st1 + 0.05f * st2;
	float fmeasure = st*sy / (st + sy);
	float score = fmeasure * ((1.f - alpha) + alpha * (1.f / logf(1.f + dn)));
#ifdef DEBUG_MATH_SCORE_INSPECT
	printf("t = %u, %u, %u, len=%u \n", t[0],t[1],t[2], qn);
	printf("st = %f, %f, %f \n", st0, st1, st2);
	printf("fmeasure = %f, score = %f \n", fmeasure, score);
#endif
	score = score * 100000.f;
	hit->score = (uint32_t)(score);
}

void
math_expr_set_score(struct math_expr_sim_factors* factor,
                    struct math_expr_score_res* hit)
{
	//math_expr_set_score_7(factor, hit);
	//math_expr_set_score_10(factor, hit);
	
	math_expr_set_score_1(factor, hit);
	//math_expr_set_score_2(factor, hit);
}

static mnc_score_t
prefix_symbolset_similarity(uint64_t cur_min, struct postmerge* pm,
                            struct math_prefix_loc *rmap, uint32_t n)
{
	struct math_posting_item_v2   *po_item;
	math_posting_t                 posting;
	struct math_pathinfo_v2        pathinfo[MAX_MATH_PATHS];
	struct subpath_ele            *subpath_ele;
	int i, j, k, m;

	/* reset mnc for scoring new document */
	mnc_reset_docs();

	for (i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			posting = pm->postings[i];
			po_item = pm->cur_pos_item[i];
			if (math_posting_pathinfo_v2(
				posting,
				po_item->pathinfo_pos,
				po_item->n_paths,
				pathinfo
			)) {
				continue;
			}

			subpath_ele = math_posting_get_ele(posting);
			for (j = 0; j <= subpath_ele->dup_cnt; j++) {
				uint32_t qr, ql;
				qr = subpath_ele->rid[j];
				ql = subpath_ele->dup[j]->path_id;

				for (m = 0; m < n; m++) {
					if (qr == rmap[m].qr) {
						for (k = 0; k < po_item->n_paths; k++) {
							uint32_t dr, dl;
							struct math_pathinfo_v2 *p = pathinfo + k;
							dr = p->subr_id;
							dl = p->leaf_id;
							if (dr == rmap[m].dr) {
								uint32_t slot;
								struct mnc_ref mnc_ref;
								mnc_ref.sym = p->lf_symb;
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
	int (*DP)[64] = calloc(64, 64 * sizeof(int));
	int lcs = 0;
	int i, j;
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 64; j++) {
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

int substring_filter(enum symbol_id *str1, enum symbol_id *str2)
{
	int i;
	for (i = 0; i < 64; i++) {
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
	struct math_posting_item_v2   *po_item;
	math_posting_t                 posting;
	struct math_pathinfo_v2        pathinfo[MAX_MATH_PATHS];
	struct subpath_ele            *subpath_ele;
	int i, j, k;

	enum symbol_id querystr[64] = {0};
	enum symbol_id candistr[64] = {0};

	for (i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			posting = pm->postings[i];
			po_item = pm->cur_pos_item[i];
			if (math_posting_pathinfo_v2(
				posting,
				po_item->pathinfo_pos,
				po_item->n_paths,
				pathinfo
			)) {
				continue;
			}

			subpath_ele = math_posting_get_ele(posting);
			for (j = 0; j <= subpath_ele->dup_cnt; j++) {
				uint32_t qn = subpath_ele->dup[j]->node_id;
				enum symbol_id qs = subpath_ele->dup[j]->lf_symbol_id;
				querystr[qn - 1] = qs;

				for (k = 0; k < po_item->n_paths; k++) {
					struct math_pathinfo_v2 *p = pathinfo + k;
					candistr[p->leaf_id - 1] = p->lf_symb;
				}
			}
		} /* end if */
	} /* end for */

	return string_longest_common_substring(querystr, candistr);
//	for (i = 0; i < 64; i++) {
//		printf("%s ", trans_symbol(querystr[i]));
//	} printf("\n");
//	for (i = 0; i < 64; i++) {
//		printf("%s ", trans_symbol(candistr[i]));
//	} printf("\n");
//	printf("lcs = %u\n", lcs);
}

static int
symbolseq_similarity(struct postmerge* pm)
{
	uint32_t                    i, j, k;
	math_posting_t              posting;
	uint32_t                    pathinfo_pos;
	struct math_posting_item   *po_item;
	struct math_pathinfo_pack  *pathinfo_pack;
	struct math_pathinfo       *pathinfo;
	struct subpath_ele         *subpath_ele;
	int ret = 0;

	enum symbol_id querystr[64] = {0};
	enum symbol_id candistr[64] = {0};

	for (i = 0; i < pm->n_postings; i++) {
		posting = pm->postings[i];
		po_item = pm->cur_pos_item[i];
		subpath_ele = math_posting_get_ele(posting);

		pathinfo_pos = po_item->pathinfo_pos;
		pathinfo_pack = math_posting_pathinfo(posting, pathinfo_pos);

		if (NULL == pathinfo_pack || NULL == subpath_ele) {
			return 0;
		}

		for (j = 0; j < pathinfo_pack->n_paths; j++) {
			pathinfo = pathinfo_pack->pathinfo + j;
			assert(pathinfo->path_id <= 64);
			candistr[pathinfo->path_id - 1] = pathinfo->lf_symb;

			for (k = 0; k <= subpath_ele->dup_cnt; k++) {
				enum symbol_id qs = subpath_ele->dup[k]->lf_symbol_id;
				uint32_t qn = subpath_ele->dup[k]->node_id;
				querystr[qn - 1] = qs;
			}
		}
	}

	if (pm->n_postings != 0) {
		ret = substring_filter(querystr, candistr);
//		if (0) {
//		//if (8325 == po_item->exp_id) {
//			printf("Query: ");
//			for (i = 0; i < 64; i++) {
//				if (querystr[i] == 0)
//					break;
//				printf("%s ", trans_symbol(querystr[i]));
//			} printf("\n");
//			printf("expr#%d, Candi: ", po_item->exp_id);
//			for (i = 0; i < 64; i++) {
//				printf("%s ", trans_symbol(candistr[i]));
//			} printf("\n");
//			printf("Return: %d\n", ret);
//		}
	}

	return ret;
}

uint32_t math_expr_doc_lr_paths(struct postmerge* pm)
{
	math_posting_t              posting;
	uint32_t                    pathinfo_pos;
	struct math_posting_item   *po_item;
	struct math_pathinfo_pack  *pathinfo_pack;
	if (pm->n_postings == 0) {
		return 0;
	}
	posting = pm->postings[0];
	po_item = pm->cur_pos_item[0];
	pathinfo_pos = po_item->pathinfo_pos;
	pathinfo_pack = math_posting_pathinfo(posting, pathinfo_pos);
	return pathinfo_pack->n_lr_paths;
}

struct math_expr_score_res
math_expr_score_on_merge(struct postmerge* pm,
                         uint32_t level, uint32_t n_qry_lr_paths)
{
	uint32_t                    i, j, k;
	math_posting_t              posting;
	uint32_t                    pathinfo_pos;
	struct math_posting_item   *po_item;
	struct math_pathinfo_pack  *pathinfo_pack;
	struct math_pathinfo       *pathinfo;
	struct subpath_ele         *subpath_ele;
	bool                        skipped = 0;
	struct math_expr_score_res  ret = {0};

	/* reset mnc for scoring new document */
	uint32_t slot;
	struct mnc_ref mnc_ref;
	mnc_reset_docs();

	for (i = 0; i < pm->n_postings; i++) {
		/* for each merged posting item from posting lists */
		posting = pm->postings[i];
		po_item = pm->cur_pos_item[i];
		subpath_ele = math_posting_get_ele(posting);

		/* get pathinfo position of corresponding merged item */
		pathinfo_pos = po_item->pathinfo_pos;

		/* use pathinfo position to get pathinfo packet */
		pathinfo_pack = math_posting_pathinfo(posting, pathinfo_pos);

		if (NULL == pathinfo_pack || NULL == subpath_ele) {
			/* unexpected read error, e.g. file is corrupted */
#ifdef DEBUG_MATH_EXPR_SEARCH
			fprintf(stderr, "pathinfo_pack or subpath_ele is NULL.\n");
#endif
			skipped = 1;
			break;
		}

		if (n_qry_lr_paths > pathinfo_pack->n_lr_paths) {
			/* impossible to match, skip this math expression */
#ifdef DEBUG_MATH_EXPR_SEARCH
			printf("query leaf-root paths (%u) is greater than "
			       "document leaf-root paths (%u), skip this expression."
			       "\n", n_qry_lr_paths, pathinfo_pack->n_lr_paths);
#endif
			skipped = 1;
			break;
		}

		for (j = 0; j < pathinfo_pack->n_paths; j++) {
			/* for each pathinfo from this pathinfo packet */
			pathinfo = pathinfo_pack->pathinfo + j;

			/* preparing to score corresponding document subpaths */
			mnc_ref.sym = pathinfo->lf_symb;
			slot = mnc_map_slot(mnc_ref);

			for (k = 0; k <= subpath_ele->dup_cnt; k++) {
				/*
				 * add this document subpath for scoring.
				 * (path_id [1, 64] is mapped to [0, 63])
				 */
				mnc_doc_add_rele(slot, pathinfo->path_id - 1,
				                 subpath_ele->dup[k]->path_id - 1);
			}
		}
	}

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("query leaf-root paths: %u\n", n_qry_lr_paths);
	printf("document leaf-root paths: %u\n", pathinfo_pack->n_lr_paths);
	printf("posting merge level: %u\n", level);
#endif

	/* finally calculate expression similarity score */
	if (!skipped && pm->n_postings != 0) {
		ret.score = mnc_score(true);
		ret.doc_id = po_item->doc_id;
		ret.exp_id = po_item->exp_id;
	}

	return ret;
}

struct math_expr_score_res
math_expr_filter_on_merge(struct postmerge* pm,
                         uint32_t level, uint32_t n_qry_lr_paths)
{
	struct math_posting_item   *po_item;
	struct math_expr_score_res  ret = {0};

	/* finally calculate expression similarity score */
	if (pm->n_postings != 0) {
		ret.score = symbolseq_similarity(pm);
		po_item = pm->cur_pos_item[0];
		ret.doc_id = po_item->doc_id;
		ret.exp_id = po_item->exp_id;
	}

	return ret;
}

struct math_expr_score_res
math_expr_prefix_score_on_merge(
	uint64_t cur_min, struct postmerge* pm,
	uint32_t n_qry_lr_paths, struct math_prefix_qry *pq,
	uint32_t srch_level, void *arg)
{
	struct math_posting_item_v2   *po_item;
	math_posting_t                 posting;
	struct math_pathinfo_v2        pathinfo[MAX_MATH_PATHS];
	struct subpath_ele            *subpath_ele;
	struct math_expr_score_res     ret = {0};
	uint32_t                       n_doc_lr_paths = 0;
	int i, j, k;

	uint32_t n_joint_nodes, topk_cnt[3] = {0};
	struct math_prefix_loc  rmap[3] = {0};
	mnc_score_t             symbol_sim = 0;

	int lcs = 0;

	for (i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			posting = pm->postings[i];
			po_item = pm->cur_pos_item[i];
			if (math_posting_pathinfo_v2(
				posting,
				po_item->pathinfo_pos,
				po_item->n_paths,
				pathinfo
			)) {
				continue;
			}

			if (po_item->n_lr_paths)
				n_doc_lr_paths = po_item->n_lr_paths;
			else
				n_doc_lr_paths = MAX_MATH_PATHS; /* handle overflow */

#ifdef DEBUG_MATH_EXPR_SEARCH
			printf("from posting[%u]: ", i);
			printf("doc#%u, exp#%u with originally %u lr_paths {\n",
			       po_item->doc_id, po_item->exp_id, n_doc_lr_paths);
#endif

			subpath_ele = math_posting_get_ele(posting);
			for (j = 0; j <= subpath_ele->dup_cnt; j++) {
				uint32_t qr, ql;
				qr = subpath_ele->rid[j];
				ql = subpath_ele->dup[j]->path_id;
#ifdef DEBUG_MATH_EXPR_SEARCH
				printf("\t qry prefix path [%u ~ %u, %s] hits: \n", qr, ql,
				       trans_symbol(subpath_ele->dup[j]->lf_symbol_id));
#endif
				for (k = 0; k < po_item->n_paths; k++) {
					uint32_t dr, dl;
					struct math_pathinfo_v2 *p = pathinfo + k;
					dr = p->subr_id;
					dl = p->leaf_id;
#ifdef DEBUG_MATH_EXPR_SEARCH
					{
						uint64_t res = 0;
						res = pq_hit(pq, qr, ql, dr, dl);
						printf("\t\t doc prefix path [%u ~ %u, %s]\n", dr, dl,
						       trans_symbol(p->lf_symb));
						printf("\t\t hit returns 0x%lu \n", res);
						//pq_print(*pq, 16);
						printf("\n");
					}
#else
					pq_hit(pq, qr, ql, dr, dl);
#endif
				}
			}
#ifdef DEBUG_MATH_EXPR_SEARCH
			printf("}\n");
#endif
		}
	}

	n_joint_nodes = pq_align(pq, topk_cnt, rmap, 3);
	pq_reset(pq);

//	printf("rmap: <%u-%u>, <%u-%u>, <%u-%u>\n",
//	       rmap[0].qr, rmap[0].dr,
//	       rmap[1].qr, rmap[1].dr,
//	       rmap[2].qr, rmap[2].dr);
	symbol_sim = prefix_symbolset_similarity(cur_min, pm, rmap, 3);

	//lcs = prefix_symbolseq_similarity(cur_min, pm);
	(void)lcs;

	if (pm->n_postings != 0) {
		struct math_expr_sim_factors factors = {
			symbol_sim, 0,
			n_qry_lr_paths, n_doc_lr_paths,
			topk_cnt, 3, n_joint_nodes,
			0
		};
		ret.doc_id = po_item->doc_id;
		ret.exp_id = po_item->exp_id;
		
#ifdef DEBUG_MATH_SCORE_INSPECT
		if (score_inspect_filter(ret, (struct indices *)arg))
#endif
			math_expr_set_score(&factors, &ret);
	}

	return ret;
}
