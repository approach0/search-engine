#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "search.h"

/*
 * query related functions.
 */

LIST_DEF_FREE_FUN(query_list_free, struct query_keyword, ln, free(p));

struct query query_new()
{
	struct query qry;
	LIST_CONS(qry.keywords);
	qry.len = 0;
	return qry;
}

void query_delete(struct query qry)
{
	query_list_free(&qry.keywords);
	qry.len = 0;
}

static LIST_IT_CALLBK(qry_keyword_print)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(fh, FILE, pa_extra);

	fprintf(fh, "[%u]: `%S'", kw->pos, kw->wstr);

	if (kw->type == QUERY_KEYWORD_TEX)
		fprintf(fh, " (tex)");

	if (pa_now->now == pa_head->last)
		fprintf(fh, ".");
	else
		fprintf(fh, ", ");

	LIST_GO_OVER;
}

void query_print_to(struct query qry, FILE* fh)
{
	fprintf(fh, "query: ");
	list_foreach(&qry.keywords, &qry_keyword_print, fh);
	fprintf(fh, "\n");
}

void query_push_keyword(struct query *qry, const struct query_keyword* kw)
{
	struct query_keyword *copy = malloc(sizeof(struct query_keyword));

	memcpy(copy, kw, sizeof(struct query_keyword));
	LIST_NODE_CONS(copy->ln);

	list_insert_one_at_tail(&copy->ln, &qry->keywords, NULL, NULL);
	copy->pos = (qry->len ++);
}

static LIST_IT_CALLBK(add_into_qry)
{
	LIST_OBJ(struct term_list_node, nd, ln);
	P_CAST(qry, struct query, pa_extra);
	struct query_keyword kw;

	kw.type = QUERY_KEYWORD_TERM;
	wstr_copy(kw.wstr, nd->term);

	query_push_keyword(qry, &kw);
	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(txt_seg_list_free, struct term_list_node,
                  ln, free(p));

void query_digest_utf8txt(struct query *qry, const char* txt)
{
	list li = text_segment(txt);
	list_foreach(&li, &add_into_qry, qry);
	txt_seg_list_free(&li);
}

/*
 * indices related functions.
 */

struct indices indices_open(const char*index_path)
{
	struct indices        indices;
	bool                  open_err = 0;
	const char            kv_db_fname[] = "kvdb-offset.bin";
	char                  term_index_path[MAX_DIR_PATH_NAME_LEN];
	struct postcache_pool postcache;

	void         *ti = NULL;
	math_index_t  mi = NULL;
	keyval_db_t   keyval_db = NULL;

	/*
	 * open term index.
	 */
	sprintf(term_index_path, "%s/term", index_path);
	mkdir_p(term_index_path);
	ti = term_index_open(term_index_path, TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
		printf("cannot create/open term index.\n");

		open_err = 1;
		goto skip;
	}

	/*
	 * open math index.
	 */
	mi = math_index_open(index_path, MATH_INDEX_READ_ONLY);
	if (mi == NULL) {
		printf("cannot create/open math index.\n");

		open_err = 1;
		goto skip;
	}

	/*
	 * open document offset key-value database.
	 */
	keyval_db = keyval_db_open_under(kv_db_fname, index_path,
	                                 KEYVAL_DB_OPEN_RD);
	if (keyval_db == NULL) {
		printf("key-value DB open error.\n");

		open_err = 1;
		goto skip;
	} else {
		printf("%lu records in key-value DB.\n",
		       keyval_db_records(keyval_db));
	}

	/* initialize posting cache pool */
	postcache_init(&postcache, POSTCACHE_POOL_LIMIT_1MB);

skip:
	indices.ti = ti;
	indices.mi = mi;
	indices.keyval_db = keyval_db;
	indices.postcache = postcache;
	indices.open_err = open_err;

	return indices;
}

void indices_cache(struct indices* indices, uint64_t mem_limit)
{
	enum postcache_err res;
	uint32_t  termN, df;
	char     *term;
	void     *posting;
	term_id_t term_id;

	postcache_set_mem_limit(&indices->postcache, mem_limit);

	termN = term_index_get_termN(indices->ti);

	printf("caching terms:\n");
	for (term_id = 1; term_id <= termN; term_id++) {
		df = term_index_get_df(indices->ti, term_id);
		term = term_lookup_r(indices->ti, term_id);
		posting = term_index_get_posting(indices->ti, term_id);

		if (posting) {
			printf("`%s'(df=%u) ", term, df);
			res = postcache_add_term_posting(&indices->postcache,
			                                 term_id, posting);
			if (res == POSTCACHE_EXCEED_MEM_LIMIT)
				break;
		}

		free(term);
	}
	printf("\n");

	printf("caching complete:\n");
	postcache_print_mem_usage(&indices->postcache);
}

void indices_close(struct indices* indices)
{
	if (indices->ti) {
		term_index_close(indices->ti);
		indices->ti = NULL;
	}

	if (indices->mi) {
		math_index_close(indices->mi);
		indices->mi = NULL;
	}

	if (indices->keyval_db) {
		keyval_db_close(indices->keyval_db);
		indices->keyval_db = NULL;
	}

	if (indices->postcache.trp_root) {
		postcache_free(&indices->postcache);
	}
}

/*
 * search related functions.
 */

static bool term_posting_jump_wrap(void *posting, uint64_t to_id)
{
	bool succ;

	/* because uint64_t value can be greater than doc_id_t,
	 * we need a wrapper function to safe-guard from
	 * calling term_posting_jump with illegal argument. */
	if (to_id >= UINT_MAX)
		succ = 0;
	else
		succ = term_posting_jump(posting, (doc_id_t)to_id);

	return succ;
}

static void *term_posting_current_wrap(void *posting)
{
	return (void*)term_posting_current(posting);
}

static uint64_t term_posting_current_id_wrap(void *item)
{
	doc_id_t doc_id;
	doc_id = ((struct term_posting_item*)item)->doc_id;
	return (uint64_t)doc_id;
}

static struct postmerge_callbks *get_memory_postmerge_callbks(void)
{
	static struct postmerge_callbks ret;
	ret.start  = &mem_posting_start;
	ret.finish = &mem_posting_finish;
	ret.jump   = &mem_posting_jump;
	ret.next   = &mem_posting_next;
	ret.now    = &mem_posting_current;
	ret.now_id = &mem_posting_current_id;

	return &ret;
}

static struct postmerge_callbks *get_disk_postmerge_callbks(void)
{
	static struct postmerge_callbks ret;
	ret.start  = &term_posting_start;
	ret.finish = &term_posting_finish;
	ret.jump   = &term_posting_jump_wrap;
	ret.next   = &term_posting_next;
	ret.now    = &term_posting_current_wrap;
	ret.now_id = &term_posting_current_id_wrap;

	return &ret;
}

struct add_postinglist_for_merge_args {
	struct indices          *indices;
	struct postmerge        *pm;
	uint32_t                 docN;
	uint32_t                 posting_idx;
	struct BM25_term_i_args *bm25args;
};

static LIST_IT_CALLBK(add_postinglist_for_merge)
{
	void                     *posting = NULL;
	struct postmerge_callbks *postmerge_callbks = NULL;
	void                     *ti;
	term_id_t                 term_id;
	uint32_t                  df;
	float                    *idf;
	float                     docN;

	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(apfm_args, struct add_postinglist_for_merge_args, pa_extra);

	/* get term index for short-hand */
	ti = apfm_args->indices->ti;

	/* get utf-8 keyword string and termID */
	char *kw_utf8 = wstr2mbstr(kw->wstr);
	term_id = term_lookup(ti, kw_utf8);

	/* get docN in float format */
	docN = (float)apfm_args->docN;

	/* set IDF pointer */
	idf = apfm_args->bm25args->idf;

	printf("adding posting list of keyword `%s'(termID: %u)...\n",
	       kw_utf8, term_id);

	if (term_id == 0) {
		/* if term is not found in our dictionary */
		posting = NULL;
		postmerge_callbks = NULL;

		/* get IDF number */
		df = 0;
		idf[apfm_args->posting_idx] = BM25_idf((float)df, docN);

		printf("keyword not found.\n");
	} else {
		/* otherwise, get on-disk or cached posting list */
		struct postcache_item *cache_item =
			postcache_find(&apfm_args->indices->postcache, term_id);

		if (NULL != cache_item) {
			/* if this term is already cached */
			posting = cache_item->posting;
			postmerge_callbks = get_memory_postmerge_callbks();

			printf("using cached posting list.\n");
		} else {
			/* otherwise read posting from disk */
			posting = term_index_get_posting(ti, term_id);
			postmerge_callbks = get_disk_postmerge_callbks();
			printf("using on-disk posting list.\n");
		}

		/* get IDF number */
		df = term_index_get_df(ti, term_id);
		idf[apfm_args->posting_idx] = BM25_idf((float)df, docN);
	}

	postmerge_posts_add(apfm_args->pm, posting, postmerge_callbks, NULL);
//	kw->type: QUERY_KEYWORD_TEX, QUERY_KEYWORD_TERM.

	/* increment current adding posting list index */
	apfm_args->posting_idx ++;

	LIST_GO_OVER;
}

static void
add_postinglists_for_merge(struct indices *indices, const struct query *qry,
                           struct postmerge *pm,
                           struct BM25_term_i_args *bm25args)
{

	/* setup argument variable `apfm_args' (in & out) */
	struct add_postinglist_for_merge_args apfm_args;
	apfm_args.indices = indices;
	apfm_args.pm = pm;
	apfm_args.docN = term_index_get_docN(indices->ti);
	apfm_args.posting_idx = 0;
	apfm_args.bm25args = bm25args;

	/* add posting list of every keyword for merge */
	list_foreach((list*)&qry->keywords, &add_postinglist_for_merge,
	             &apfm_args);

	/* prepare some score parameters */
	bm25args->n_postings = apfm_args.posting_idx;
	bm25args->avgDocLen = (float)term_index_get_avgDocLen(indices->ti);
	bm25args->b  = BM25_DEFAULT_B;
	bm25args->k1 = BM25_DEFAULT_K1;
	bm25args->frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25args->avgDocLen;

	printf("BM25 arguments:\n");
	BM25_term_i_args_print(bm25args);
}

struct posting_merge_extra_args {
	struct indices          *indices;
	struct BM25_term_i_args *bm25args;
	struct rank_set         *rk_set;
};

void
posting_on_merge(uint64_t cur_min, struct postmerge* pm, void* extra_args)
{
//	float score = 0.f;
//	doc_id_t docID = cur_min;

	return;
}

struct rank_set
indices_run_query(struct indices *indices, const struct query qry)
{
	struct postmerge                postmerge;
	struct BM25_term_i_args         bm25args;
	struct rank_set                 rk_set;
	struct posting_merge_extra_args pm_args;

	/* initialize postmerge */
	postmerge_posts_clear(&postmerge);
	add_postinglists_for_merge(indices, &qry, &postmerge, &bm25args);

	/* initialize ranking set */
	rank_set_init(&rk_set, RANK_SET_DEFAULT_VOL);

	/* setup merge extra arguments */
	pm_args.indices  = indices;
	pm_args.bm25args = &bm25args;
	pm_args.rk_set   = &rk_set;

	/* posting list merge */
	printf("start merging...\n");
	if (!posting_merge(&postmerge, POSTMERGE_OP_AND,
	                   &posting_on_merge, &pm_args))
		fprintf(stderr, "posting list merge failed.\n");

	/* rank top K hits */
	rank_sort(&rk_set);

	/* return ranking set */
	return rk_set;
}
