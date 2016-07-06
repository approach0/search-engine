#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

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
	LIST_OBJ(struct text_seg, seg, ln);
	P_CAST(qry, struct query, pa_extra);
	struct query_keyword kw;

	kw.type = QUERY_KEYWORD_TERM;
	wstr_copy(kw.wstr, mbstr2wstr(seg->str));

	query_push_keyword(qry, &kw);
	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(txt_seg_list_free, struct text_seg,
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

	void                 *ti = NULL;
	math_index_t          mi = NULL;
	keyval_db_t           keyval_db = NULL;

	struct postcache_pool postcache;
	postcache.trp_root = NULL;

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
	bool      ellp_lock = 0;

	postcache_set_mem_limit(&indices->postcache, mem_limit);

	termN = term_index_get_termN(indices->ti);

	printf("caching terms:\n");
	for (term_id = 1; term_id <= termN; term_id++) {
		df = term_index_get_df(indices->ti, term_id);
		term = term_lookup_r(indices->ti, term_id);
		posting = term_index_get_posting(indices->ti, term_id);

		if (posting) {
			if (term_id < MAX_PRINT_CACHE_TERMS) {
				printf("`%s'(df=%u) ", term, df);

			} else if (!ellp_lock) {
				printf(" ...... ");
				ellp_lock = 1;
			}

			res = postcache_add_term_posting(&indices->postcache,
			                                 term_id, posting);
			if (res == POSTCACHE_EXCEED_MEM_LIMIT)
				break;
		}

		free(term);
	}
	printf("\n");

	printf("caching complete (%u posting lists cached):\n", term_id);
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

struct add_postinglist_arg {
	struct indices          *indices;
	struct postmerge        *pm;
	uint32_t                 docN;
	uint32_t                 posting_idx;
	struct BM25_term_i_args *bm25args;
};

struct add_postinglist {
	void *posting;
	struct postmerge_callbks *postmerge_callbks;
};

static struct add_postinglist
term_postinglist(char *kw_utf8, struct add_postinglist_arg *apfm_args)
{
	struct add_postinglist ret;
	void                  *ti;
	term_id_t              term_id;
	uint32_t               df;
	float                 *idf;
	float                  docN;

	/* get variables for short-hand */
	ti = apfm_args->indices->ti;
	term_id = term_lookup(ti, kw_utf8);
	docN = (float)apfm_args->docN;
	idf = apfm_args->bm25args->idf;

	printf("posting list[%u] of keyword `%s'(termID: %u)...\n",
	       apfm_args->posting_idx, kw_utf8, term_id);

	if (term_id == 0) {
		/* if term is not found in our dictionary */
		ret.posting = NULL;
		ret.postmerge_callbks = NULL;

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
			ret.posting = cache_item->posting;
			ret.postmerge_callbks = get_memory_postmerge_callbks();

			printf("using cached posting list.\n");
		} else {
			/* otherwise read posting from disk */
			ret.posting = term_index_get_posting(ti, term_id);
			ret.postmerge_callbks = get_disk_postmerge_callbks();
			printf("using on-disk posting list.\n");
		}

		/* get IDF number */
		df = term_index_get_df(ti, term_id);
		idf[apfm_args->posting_idx] = BM25_idf((float)df, docN);
	}

	return ret;
}

struct math_score_merge_arg {
	struct mem_posting *mem_po;      /* output */
	uint32_t            doc_score;   /* sum of expression scores */
	doc_id_t            last_doc_id; /* document that is summing score */
};

struct math_doc_score {
	doc_id_t doc_id;
	uint32_t score;
};

static void write_math_doc_sum_score(struct math_score_merge_arg *msm_arg)
{
	struct math_doc_score mds;
	mds.doc_id = msm_arg->last_doc_id;
	mds.score = msm_arg->doc_score;
	mem_posting_write(msm_arg->mem_po, mds.doc_id,
	                  &mds, sizeof(struct math_doc_score));

#ifdef DEBUG_MIX_SCORING
	printf("math-search docID#%u sum-score: %u\n", mds.doc_id, mds.score);
#endif
}

static void
math_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	struct math_score_res res;

	/* get additional math score arguments */
	P_CAST(mes_arg, struct math_extra_score_arg, extra_args);
	P_CAST(msm_arg, struct math_score_merge_arg, mes_arg->extra_search_args);

	/* calculate math similarity on merge */
	res = math_score_on_merge(pm, mes_arg->dir_merge_level,
	                          mes_arg->n_qry_lr_paths);

	if (res.doc_id != msm_arg->last_doc_id) {
		/* write old document final score to in-memory posting list */
		if (msm_arg->doc_score > 0)
		write_math_doc_sum_score(msm_arg);

		/* clear old document score */
		msm_arg->last_doc_id = res.doc_id;
		msm_arg->doc_score = 0;
	}

	/* sum expression scores */
	msm_arg->doc_score += res.score;

#ifdef DEBUG_PRINT_MATH_HITS
	printf("math-search hit: docID#%u expID#%u score: %u\n",
		   res.doc_id, res.exp_id, res.score);
#endif
}

static struct add_postinglist
math_postinglist(char *kw_utf8, struct add_postinglist_arg *apfm_args)
{
	struct add_postinglist      ret;
	struct math_score_merge_arg msm_arg;

	math_index_t mi = apfm_args->indices->mi;

	printf("posting list[%u] of tex `%s'...\n",
	       apfm_args->posting_idx, kw_utf8);

	/* a temporary memory posting to store intermediate math-search results */
	msm_arg.mem_po = mem_posting_create(sizeof(struct math_doc_score),
	                                    MAX_SKIPPY_SPANS);
	msm_arg.doc_score = 0;
	msm_arg.last_doc_id = 0;

	math_search_posting_merge(mi, kw_utf8, DIR_MERGE_DEPTH_FIRST,
	                          &math_posting_on_merge, &msm_arg);

	/* write the final math-document sum-score */
	if (msm_arg.doc_score > 0)
		write_math_doc_sum_score(&msm_arg);

	ret.posting = msm_arg.mem_po;
	ret.postmerge_callbks = get_memory_postmerge_callbks();
	return ret;
}

static LIST_IT_CALLBK(add_postinglist_for_merge)
{
	struct add_postinglist res;
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(apfm_args, struct add_postinglist_arg, pa_extra);

	switch (kw->type) {
	case QUERY_KEYWORD_TERM:
		res = term_postinglist(wstr2mbstr(kw->wstr), apfm_args);
		break;

	case QUERY_KEYWORD_TEX:
		res = math_postinglist(wstr2mbstr(kw->wstr), apfm_args);
		break;

	default:
		assert(0);
	}

	/* add posting list for merge */
	postmerge_posts_add(apfm_args->pm, res.posting, res.postmerge_callbks,
	                    &kw->type);

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
	struct add_postinglist_arg apfm_args;
	apfm_args.indices = indices;
	apfm_args.pm = pm;
	apfm_args.docN = term_index_get_docN(indices->ti);
	apfm_args.posting_idx = 0;
	apfm_args.bm25args = bm25args;

	/* add posting list of every keyword for merge */
	list_foreach((list*)&qry->keywords, &add_postinglist_for_merge,
	             &apfm_args);

	/* prepare some term-posting list score parameters */
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

static void
mixed_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                       void* extra_args)
{
	uint32_t i;
	float term_score, score = 0.f;
	enum query_kw_type *type;
	doc_id_t docID = cur_min;

	P_CAST(pm_args, struct posting_merge_extra_args, extra_args);
	float doclen = (float)term_index_get_docLen(pm_args->indices->ti, docID);
	struct term_posting_item *tpi;
	struct math_doc_score    *mds;

	for (i = 0; i < pm->n_postings; i++)
		if (pm->curIDs[i] == cur_min) {
			type = (enum query_kw_type *)pm->posting_args[i];

			switch (*type) {
			case QUERY_KEYWORD_TERM:
				tpi = (struct term_posting_item *)pm->cur_pos_item[i];
				term_score = BM25_term_i_score(pm_args->bm25args,
				                               i, tpi->tf, doclen);
				score += term_score;

#ifdef DEBUG_MIX_SCORING
				printf("score += posting[%u].BM25_score(tf=%u, docLen=%f) "
				       "= %f\n", i, tpi->tf, doclen, term_score);
#endif
				break;

			case QUERY_KEYWORD_TEX:
				mds = (struct math_doc_score *)pm->cur_pos_item[i];
				score += (float)mds->score;

#ifdef DEBUG_MIX_SCORING
				printf("score += posting[%u].math_doc_sum_score = %u\n",
				       i, mds->score);
#endif
				break;

			default:
				assert(0);
			}
		}

	rank_set_hit(pm_args->rk_set, docID, score);

#ifdef DEBUG_MIX_SCORING
	printf("total score of doc#%u = %f\n", docID, score);
#endif

	return;
}

void free_mixed_postinglists(struct postmerge *pm)
{
	uint32_t i;
	enum query_kw_type *type;
	struct mem_posting *mem_po;

	for (i = 0; i < pm->n_postings; i++) {
		mem_po = (struct mem_posting*)pm->postings[i];
		type = (enum query_kw_type *)pm->posting_args[i];

		if (*type == QUERY_KEYWORD_TEX) {
			printf("releasing math in-memory posting[%u] (%u blocks)...\n",
			       i, mem_po->n_tot_blocks);

			mem_posting_release(mem_po);
		}
	}
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
	if (!posting_merge(&postmerge, POSTMERGE_OP_OR,
	                   &mixed_posting_on_merge, &pm_args))
		fprintf(stderr, "posting list merge failed.\n");

	/* free posting lists */
	free_mixed_postinglists(&postmerge);

	/* rank top K hits */
	rank_sort(&rk_set);

	/* return ranking set */
	return rk_set;
}
