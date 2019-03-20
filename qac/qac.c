#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "qac.h"

struct qac_index {
	math_index_t mi;
	blob_index_t bi_tex_str;
	blob_index_t bi_tex_info;
};

uint32_t qac_index_touch(void *qi_, uint32_t plus)
{
	struct qac_index *qi = (struct qac_index*)qi_;
	struct qac_tex_info *head_tex_info = NULL;
	uint32_t total_freq = 0;
	struct qac_tex_info tex_info = {0, 0};

	if (qi->bi_tex_str == NULL || qi->bi_tex_info == NULL)
		return total_freq;

	(void)blob_index_read(qi->bi_tex_info, 0, (void **)&head_tex_info);
	if (head_tex_info) {
		total_freq = head_tex_info->freq;
		free(head_tex_info);
		total_freq += plus;
		tex_info.freq = total_freq;
		blob_index_replace(qi->bi_tex_info, 0, &tex_info);
	} else {
		blob_index_append(qi->bi_tex_str, 0, "ALL", 3 + 1);
		blob_index_append(qi->bi_tex_info, 0, &tex_info, sizeof(tex_info));
	}

#ifdef QAC_INDEX_DEBUG_PRINT
	(void)blob_index_read(qi->bi_tex_info, 0, (void **)&head_tex_info);
	printf("(%u, %u)\n", head_tex_info->freq, head_tex_info->lr_paths);
	free(head_tex_info);
#endif

	return total_freq;
}

void *qac_index_open(char *path, enum qac_index_open_opt opt)
{
	char blob_file_path[8024];

	struct qac_index *qi = malloc(sizeof(struct qac_index));
	qi->mi = math_index_open(path,
		(opt == QAC_INDEX_WRITE_READ) ? MATH_INDEX_WRITE: MATH_INDEX_READ_ONLY
	);

	if (qi->mi == NULL) {
		printf("cannot open index.\n");
		return NULL;
	}

	sprintf(blob_file_path, "%s/%s", qi->mi->dir, "uniq_tex_str");
	qi->bi_tex_str = blob_index_open(blob_file_path,
		(opt == QAC_INDEX_WRITE_READ) ? BLOB_OPEN_RMA_RANDOM : BLOB_OPEN_RD);
	if (qi->bi_tex_str == NULL) {
		printf("cannot open index.\n");
		return NULL;
	}

	sprintf(blob_file_path, "%s/%s", qi->mi->dir, "uniq_tex_info");
	qi->bi_tex_info = blob_index_open(blob_file_path,
		(opt == QAC_INDEX_WRITE_READ) ? BLOB_OPEN_RMA_RANDOM : BLOB_OPEN_RD);
	if (qi->bi_tex_info == NULL) {
		printf("cannot open index.\n");
		return NULL;
	}

	qac_index_touch(qi, 0);
	return qi;
}

void qac_index_close(void *qi_)
{
	struct qac_index *qi = (struct qac_index*)qi_;
	math_index_close(qi->mi);
	blob_index_close(qi->bi_tex_str);
	blob_index_close(qi->bi_tex_info);
	free(qi);
}

static int
set_touch_id_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	P_CAST(mesa, struct math_extra_score_arg, extra_args);
	P_CAST(touchID, uint32_t, mesa->expr_srch_arg);

	/* tree leaves */
	uint32_t n_qry_lr_paths = mesa->n_qry_lr_paths;
	uint32_t n_doc_lr_paths = math_expr_doc_lr_paths(pm);

	if (n_qry_lr_paths == 1)
		return 1; /* stop searching */

	if (n_qry_lr_paths != n_doc_lr_paths)
		return 0; /* continue searching */

	/* compute similarity */
	struct math_expr_score_res sim_res;
	sim_res = math_expr_score_on_merge(pm, mesa->dir_merge_level,
	                                   mesa->n_qry_lr_paths);
	uint32_t nsim;
	nsim = (sim_res.score * MAX_MATH_EXPR_SIM_SCALE) /
	       (n_qry_lr_paths * MNC_MARK_FULL_SCORE);

//	/* tex string */
//	po_item = pm->cur_pos_item[0];
//	tex_info = math_qac_get(args->qi, po_item->exp_id, &tex);
//	assert(NULL != tex);
//#ifdef QAC_INDEX_DEBUG_PRINT
//	printf("doc#%u, exp#%u, score: %u.\n",
//	       res.doc_id, res.exp_id, res.score);
//#endif

	if (nsim == MAX_MATH_EXPR_SIM_SCALE) {
		*touchID = sim_res.exp_id;
		return 1; /* stop searching */
	}
	return 0;
}

uint32_t math_qac_index_uniq_tex(qac_index_t *qi_, const char *tex)
{
	struct qac_index *qi = (struct qac_index*)qi_;
	struct tex_parse_ret parse_ret;

#define QAC_INDEX_UNIQUE_EXPR
#ifdef QAC_INDEX_UNIQUE_EXPR
	uint32_t touchID = 0;

	math_expr_search(qi->mi, (char *)tex, MATH_SRCH_EXACT_STRUCT,
	                 &set_touch_id_on_merge, &touchID);

	if (touchID) {
		struct qac_tex_info *tex_info = NULL;
		(void)blob_index_read(qi->bi_tex_info, touchID, (void **)&tex_info);

		assert(tex_info != NULL);
		tex_info->freq ++;
		blob_index_replace(qi->bi_tex_info, touchID, tex_info);
//#ifdef QAC_INDEX_DEBUG_PRINT
		if (tex_info->freq > 10)
			printf("freq expr: [%u] `%s'.\n", tex_info->freq, tex);
//#endif
		free(tex_info);
		return touchID;
	}
#endif

	parse_ret = tex_parse(tex, 0, false, false);

	if (parse_ret.code != PARSER_RETCODE_ERR) {
		uint32_t n_lr_paths = parse_ret.subpaths.n_lr_paths;
		uint32_t texID = 0;
		if (n_lr_paths == 1)
			goto skip;

		struct qac_tex_info tex_info = {1, n_lr_paths};
		texID = qac_index_touch(qi, 1);

		blob_index_append(qi->bi_tex_str, texID, tex, strlen(tex) + 1);
		blob_index_append(qi->bi_tex_info, texID, &tex_info, sizeof(tex_info));

#ifdef QAC_INDEX_DEBUG_PRINT
		printf("added as #%u\n", texID);
#endif
		math_index_add_tex(qi->mi, 1 /* always docID#1 */, texID,
		                   parse_ret.subpaths, MATH_INDEX_WO_PREFIX);
skip:
		subpaths_release(&parse_ret.subpaths);
		return texID;
	} else {
		printf("parser error: %s\n", parse_ret.msg);
		return 0;
	}
}

struct qac_tex_info math_qac_get(qac_index_t* qi_, uint32_t texID, char **tex)
{
	struct qac_tex_info *tex_info, ret = {0, 0};
	struct qac_index *qi = (struct qac_index*)qi_;

	(void)blob_index_read(qi->bi_tex_info, texID, (void **)&tex_info);
	if (tex_info) {
		(void)blob_index_read(qi->bi_tex_str, texID, (void **)tex);
		ret = *tex_info;
		free(tex_info);
	} else {
		*tex = NULL;
	}
	return ret;
}

struct qac_on_merge_args {
	ranked_results_t *rk_res;
	struct qac_index *qi;
};

static int
qac_suggestion_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	P_CAST(mesa, struct math_extra_score_arg, extra_args);
	P_CAST(args, struct qac_on_merge_args, mesa->expr_srch_arg);
	struct math_posting_item *po_item = pm->cur_pos_item[0];
	struct qac_tex_info tex_info;
	char *tex;

	/* similarity */
	struct math_expr_score_res sim_res;
	sim_res = math_expr_filter_on_merge(pm, mesa->dir_merge_level,
	                                    mesa->n_qry_lr_paths);
	/* exclude zero scored hits */
	if (sim_res.score == 0)
		return 0;

	/* suggestion score */
	tex_info = math_qac_get(args->qi, po_item->exp_id, &tex);
	assert(NULL != tex);
#if 0
	/* get tex string and frequency */
	printf("tex#%u `%s' freq: %.1f, level: %.1f, sim: %.2f => score: %.2f\n",
	       po_item->exp_id, tex, freq, level, sim, score);
#endif
	free(tex);

	uint32_t n_qry_lr_paths = mesa->n_qry_lr_paths;
	uint32_t n_doc_lr_paths = math_expr_doc_lr_paths(pm);
//	uint32_t int_sim;
//	int_sim = (sim_res.score * MAX_MATH_EXPR_SIM_SCALE) /
//		      (n_qry_lr_paths * MNC_MARK_FULL_SCORE);
//	float sim = (float)int_sim;
	float level = (float)mesa->dir_merge_level;
	if (level == 0 && n_qry_lr_paths == n_doc_lr_paths)
		return 0;

	float freq = (float)tex_info.freq;
	float score = freq / logf(1.f + (float)n_doc_lr_paths);
	consider_top_K(args->rk_res, po_item->exp_id, score, NULL, 0);
	return 0;
}

ranked_results_t math_qac_query(qac_index_t* qi_, const char *tex)
{
	struct qac_index *qi = (struct qac_index*)qi_;
	ranked_results_t rk_res;
	struct qac_on_merge_args args = {&rk_res, qi};

	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

	printf("searching %s\n", tex);
	math_expr_search(qi->mi, (char*)tex, MATH_SRCH_SUBEXPRESSION,
	                 &qac_suggestion_on_merge, &args);

	priority_Q_sort(&rk_res);

	return rk_res;
}
