#include <stdlib.h>
#include <stdio.h>

#include "tex-parser/head.h"
#include "config.h"
#include "math-expr-search.h"

uint32_t math_expr_sim(
	mnc_score_t mnc_score, uint32_t depth_delta, uint32_t qry_lr_paths,
	uint32_t doc_lr_paths, uint32_t joint_nodes, int lcs)
{
	uint32_t breath_delta = abs(doc_lr_paths - qry_lr_paths);
	uint32_t norm_mnc_score = (mnc_score * MAX_MATH_EXPR_SIM_SCALE) /
	                          (qry_lr_paths * (1 + MNC_MARK_SCORE));
	uint32_t score;
	//= norm_mnc_score / (depth_delta + breath_delta + 1);
	score = 10000 * norm_mnc_score + 100 * (joint_nodes + lcs);
	score = score / (breath_delta + 1);

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("norm_mnc = %u, depth = %u, qry, doc lr_paths = %u, %u, "
	       "joints = %u, lcs = %u, final score = %u \n",
	       norm_mnc_score, depth_delta, qry_lr_paths, doc_lr_paths,
		   joint_nodes, lcs, score);
#endif
	return score;
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
//								printf("prefix MNC: add <ql%u ~ dl%u>: %s "
//								       "@slot%u\n", ql, dl,
//								       trans_symbol(p->lf_symb), slot);
							}
						}
					}
				}
			}
		} /* end if */
	} /* end for */

	return mnc_score(false);
}

static int
prefix_symbolseq_similarity(uint64_t cur_min, struct postmerge* pm)
{
	struct math_posting_item_v2   *po_item;
	math_posting_t                 posting;
	struct math_pathinfo_v2        pathinfo[MAX_MATH_PATHS];
	struct subpath_ele            *subpath_ele;
	int i, j, k;

	int lcs = 0;
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

	{
		int (*DP)[64] = calloc(64, 64 * sizeof(int));
		for (i = 0; i < 64; i++) {
			for (j = 0; j < 64; j++) {
				if (i == 0 || j == 0) {
					DP[i][j] = 0;
				} else if (querystr[i-1] == candistr[j-1] &&
				           querystr[i-1] != 0) {
					DP[i][j] = DP[i-1][j-1] + 1;
					if (DP[i][j] > lcs)
						lcs = DP[i][j];
				} else {
					DP[i][j] = 0;
				}
			}
		}
		free(DP);
	}

//	for (i = 0; i < 64; i++) {
//		printf("%s ", trans_symbol(querystr[i]));
//	} printf("\n");
//	for (i = 0; i < 64; i++) {
//		printf("%s ", trans_symbol(candistr[i]));
//	} printf("\n");
//	printf("lcs = %u\n", lcs);

	return lcs;
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

	int lcs = 0;
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
			candistr[pathinfo->path_id - 1] = pathinfo->lf_symb;

			for (k = 0; k <= subpath_ele->dup_cnt; k++) {
				enum symbol_id qs = subpath_ele->dup[k]->lf_symbol_id;
				uint32_t qn = subpath_ele->dup[k]->node_id;
				querystr[qn - 1] = qs;
			}
		}
	}

	{
		int (*DP)[64] = calloc(64, 64 * sizeof(int));
		for (i = 0; i < 64; i++) {
			for (j = 0; j < 64; j++) {
				if (i == 0 || j == 0) {
					DP[i][j] = 0;
				} else if (querystr[i-1] == candistr[j-1] &&
				           querystr[i-1] != 0) {
					DP[i][j] = DP[i-1][j-1] + 1;
					if (DP[i][j] > lcs)
						lcs = DP[i][j];
				} else {
					DP[i][j] = 0;
				}
			}
		}
		free(DP);
	}

//	for (i = 0; i < 64; i++) {
//		printf("%s ", trans_symbol(querystr[i]));
//	} printf("\n");
//	for (i = 0; i < 64; i++) {
//		printf("%s ", trans_symbol(candistr[i]));
//	} printf("\n");
//	printf("lcs = %u\n", lcs);

	return lcs;
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
	int                         lcs;

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

	lcs = symbolseq_similarity(pm);

	/* finally calculate expression similarity score */
	if (!skipped && pm->n_postings != 0) {
		ret.score = math_expr_sim(mnc_score(true), level, n_qry_lr_paths,
		                          pathinfo_pack->n_lr_paths, 0, lcs);
		ret.doc_id = po_item->doc_id;
		ret.exp_id = po_item->exp_id;
	}

	return ret;
}

struct math_expr_score_res
math_expr_prefix_score_on_merge(
	uint64_t cur_min, struct postmerge* pm,
	uint32_t n_qry_lr_paths, struct math_prefix_qry *pq,
	uint32_t srch_level)
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
	mnc_score_t             symbol_sim;

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

			n_doc_lr_paths = po_item->n_lr_paths;

#ifdef DEBUG_MATH_EXPR_SEARCH
			printf("from posting[%u]: ", i);
			printf("doc#%u, exp#%u with originally %u lr_paths {\n",
			       po_item->doc_id, po_item->exp_id, po_item->n_lr_paths);
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

	// lcs = prefix_symbolseq_similarity(cur_min, pm);

#ifdef DEBUG_MATH_EXPR_SEARCH
	printf("sim:%u, joint nodes:%u, topk_cnt: %u, %u, %u\n",
	       symbol_sim, n_joint_nodes,
	       topk_cnt[0], topk_cnt[1], topk_cnt[2]);
	printf("\n");
#endif

	if (pm->n_postings != 0) {
		ret.score = math_expr_sim(symbol_sim, srch_level, n_qry_lr_paths,
		                          n_doc_lr_paths, n_joint_nodes, lcs);
//		ret.score = (topk_cnt[0] * 10 + n_joint_nodes) * 10000
//		          + topk_cnt[1] * 100
//		          + topk_cnt[2];
		ret.doc_id = po_item->doc_id;
		ret.exp_id = po_item->exp_id;
	}

	return ret;
}
