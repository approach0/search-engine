#include "mhook/mhook.h"
#include "common/common.h"
#include "wstring/wstring.h"
#include "indices-v3/indices.h"

#include "config.h"
#include "term-qry.h"
#include "math-search.h"
#include "search.h"
#include "proximity.h"

/* shorthands for level-2 math iterator */
#define math_iter_cur  math_l2_invlist_iter_cur;
#define math_iter_next math_l2_invlist_iter_next;
#define math_iter_skip math_l2_invlist_iter_skip;
#define math_iter_read math_l2_invlist_iter_read;

static int
prepare_term_keywords(struct indices *indices, struct query *qry,
	 struct merge_set *ms, struct BM25_scorer *bm25,
	 struct term_qry *term_qry, struct term_invlist_entry_reader *term_reader)
{
	/* initialize term query structures */
	int n_term = 0;
	for (int i = 0; i < qry->len; i++) {
		struct query_keyword *kw = query_get_kw(qry, i);
		char *kw_str = wstr2mbstr(kw->wstr); /* utf-8 */

		if (kw->type == QUERY_KEYWORD_TERM) {
			term_qry_prepare(indices->ti, kw_str, term_qry + n_term);
			n_term ++;
		}
	}

	/* merge term query duplicate keywords */
	n_term = term_qry_array_merge(term_qry, n_term);

	/* prepare Okapi-BM25 scoring structure */
	*bm25 = prepare_bm25(indices->ti, term_qry, n_term);
#ifdef PRINT_SEARCH_QUERIES
	printf("[BM25 parameters]\n");
	BM25_params_print(bm25);
	printf("\n[inverted lists]\n");
#endif

	/* only generate iterators for unique keywords */
	for (int i = 0; i < n_term; i++) {
		const int n = ms->n;
		uint32_t term_id = term_qry[i].term_id;
		struct term_invlist_entry_reader *reader = term_reader + i;

		/* lookup index entry for this term */
		*reader = term_index_lookup(indices->ti, term_id);

#ifdef PRINT_SEARCH_QUERIES
		printf("[%d] ", n);
#endif
		/* push into merge set */
		if (reader->inmemo_reader) {
#ifdef PRINT_SEARCH_QUERIES
			printf("(in memo) ");
#endif
			ms->iter  [n] = reader->inmemo_reader;
			ms->upp   [n] = term_qry[i].upp;
			ms->sortby[n] = term_qry[i].upp;
			ms->cur   [n] = (merger_callbk_cur) invlist_iter_curkey;
			ms->next  [n] = (merger_callbk_next)invlist_iter_next;
			ms->skip  [n] = (merger_callbk_skip)invlist_iter_jump;
			ms->read  [n] = (merger_callbk_read)invlist_iter_read;
		} else if (reader->ondisk_reader) {
#ifdef PRINT_SEARCH_QUERIES
			printf("(on disk) ");
#endif
			ms->iter  [n] = reader->ondisk_reader;
			ms->upp   [n] = term_qry[i].upp;
			ms->sortby[n] = term_qry[i].upp;
			ms->cur   [n] = (merger_callbk_cur) term_posting_cur;
			ms->next  [n] = (merger_callbk_next)term_posting_next;
			ms->skip  [n] = (merger_callbk_skip)term_posting_jump;
			ms->read  [n] = (merger_callbk_read)term_posting_read;
		} else {
#ifdef PRINT_SEARCH_QUERIES
			printf("( empty ) ");
#endif
			ms->iter  [n] = NULL;
			ms->upp   [n] = 0;
			ms->sortby[n] = 0;
			ms->cur   [n] = empty_invlist_cur;
			ms->next  [n] = empty_invlist_next;
			ms->skip  [n] = empty_invlist_skip;
			ms->read  [n] = empty_invlist_read;
		}

#ifdef PRINT_SEARCH_QUERIES
		term_qry_print(term_qry + i);
#endif
		ms->n += 1;
	}

	return n_term;
}

static int
prepare_math_keywords(struct indices *indices, struct query *qry,
	 struct merge_set *ms, struct math_l2_invlist **math_invlist,
	math_l2_invlist_iter_t *math_iter, float *threshold, float *dynm_thresh)
{
	int n_math = 0;
	for (int i = 0; i < qry->len; i++) {
		const int n = ms->n;
		struct query_keyword *kw = query_get_kw(qry, i);

		/* only consider math keyword here */
		if (kw->type != QUERY_KEYWORD_TEX)
			continue;

		/* math level-2 inverted list and iterator */
		char *kw_str = wstr2mbstr(kw->wstr); /* utf-8 */
		struct math_l2_invlist *minv = math_l2_invlist(indices->mi, kw_str,
			threshold + n_math, dynm_thresh + n_math);
		math_l2_invlist_iter_t miter = NULL;

#ifdef PRINT_SEARCH_QUERIES
		printf("[%d] ", n);
		if (minv) {
			printf("(level 2) ");
			math_qry_print(&minv->mq, 0);
		} else {
			printf("( empty ) `%s' (malformed TeX)\n", kw_str);
		}
#endif
		/* generate math iterator only if math invlist is valid */
		if (minv) {
			miter = math_l2_invlist_iterator(minv);
			math_invlist[n_math] = minv;
			math_iter[n_math] = miter;
		}

		/* push into merge set */
		if (minv == NULL || miter == NULL) {
			//prerr("math keyword malformed: `%s', leave empty.", kw_str);
			ms->iter  [n] = NULL;
			ms->upp   [n] = 0;
			ms->sortby[n] = 0;
			ms->cur   [n] = empty_invlist_cur;
			ms->next  [n] = empty_invlist_next;
			ms->skip  [n] = empty_invlist_skip;
			ms->read  [n] = empty_invlist_read;

		} else {
			ms->iter  [n] = miter;
			ms->upp   [n] = math_l2_invlist_iter_upp(miter);
			ms->sortby[n] = -(ms->upp[n]); /* force math keywords at bottom */
			ms->cur   [n] = (merger_callbk_cur) math_iter_cur;
			ms->next  [n] = (merger_callbk_next)math_iter_next;
			ms->skip  [n] = (merger_callbk_skip)math_iter_skip;
			ms->read  [n] = (merger_callbk_read)math_iter_read;
		}

		n_math += 1;
		ms->n += 1;
	}

	return n_math;
}

ranked_results_t
indices_run_query(struct indices* indices, struct query* qry)
{
	/* top-K results' heap */
	ranked_results_t rk_res;
	priority_Q_init(&rk_res, DEFAULT_N_TOP_RESULTS);

	/* merge set */
	struct merge_set merge_set = {0};

	/* proximity is only used in text query */
	//prox_input_t prox[qry->n_term];

	/*
	 * Prepare term query keywords
	 */
	struct term_qry term_qry[qry->n_term];
	struct term_invlist_entry_reader term_reader[qry->n_term];
	/* items are used for storing positions temporarily */
	///// struct term_posting_item term_item[qry->n_term];

	struct BM25_scorer bm25;
	int n_term = prepare_term_keywords(indices, qry, &merge_set,
	                                   &bm25, term_qry, term_reader);

	/*
	 * Prepare math query keywords
	 */
	struct math_l2_invlist *math_invlist[qry->n_math];
	math_l2_invlist_iter_t math_iter[qry->n_math];
	float threshold[qry->n_math];
	float dynm_threshold[qry->n_math];

	memset(math_invlist, 0, sizeof math_invlist);
	memset(math_iter, 0, sizeof math_iter);
	memset(threshold, 0, sizeof threshold);
	memset(dynm_threshold, 0, sizeof dynm_threshold);

	int n_math = prepare_math_keywords(indices, qry, &merge_set,
		math_invlist, math_iter, threshold, dynm_threshold);

	/*
	 * Release term query keywords
	 */
	for (int i = 0; i < n_term; i++) {
		/* free term query reader */
		struct term_invlist_entry_reader *reader = term_reader + i;
		if (reader->inmemo_reader)
			invlist_iter_free(reader->inmemo_reader);
		else if (reader->ondisk_reader)
			term_posting_finish(reader->ondisk_reader);

		term_qry_release(term_qry + i);
	}

	/*
	 * Release math query keywords
	 */
	for (int i = 0; i < n_math; i++) {
		struct math_l2_invlist *minv = math_invlist[i];
		math_l2_invlist_iter_t miter = math_iter[i];
		if (miter)
			math_l2_invlist_iter_free(miter);
		if (minv)
			math_l2_invlist_free(minv);
	}

	/* before returning top-K, invoke heap-sort. */
	priority_Q_sort(&rk_res);
	return rk_res;
}
