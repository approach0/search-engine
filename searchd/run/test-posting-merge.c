#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>

#include "list/list.h"
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "math-index/subpath-set.h"
#include "keyval-db/keyval-db.h"
#include "postmerge.h"
#include "bm25-score.h"
#include "rank.h"
#include "snippet.h"

#include "txt-seg/txt-seg.h"
#include "txt-seg/config.h"
#include "indexer/doc-term-pos.h"

struct merge_extra_arg {
	void                    *term_index;
	struct BM25_term_i_args *bm25args;
	keyval_db_t              keyval_db;
	struct rank_set         *rk_set;
};

void *term_posting_current_wrap(void *posting)
{
	return (void*)term_posting_current(posting);
}

void* math_posting_current_wrap(math_posting_t po_)
{
	return (void*)math_posting_current(po_);
}

uint64_t term_posting_current_id_wrap(void *item)
{
	doc_id_t doc_id;
	doc_id = ((struct term_posting_item*)item)->doc_id;
	return (uint64_t)doc_id;
}

uint64_t math_posting_current_id_wrap(void *po_item_)
{
	uint64_t *id64 = (uint64_t *)po_item_;
	return *id64;
};

bool term_posting_jump_wrap(void *posting, uint64_t to_id)
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

void
term_posting_on_merge(uint64_t cur_min, struct postmerge_arg* pm_arg,
                      void* extra_args)
{
	uint32_t i, hit_terms = 0;
	float score = 0.f;
	doc_id_t docID = cur_min;

	P_CAST(me_arg, struct merge_extra_arg, extra_args);
	float doclen = (float)term_index_get_docLen(me_arg->term_index, docID);
	struct term_posting_item *tpi;

	for (i = 0; i < pm_arg->n_postings; i++)
		if (pm_arg->curIDs[i] == cur_min) {
			//printf("merge docID#%lu from posting[%d]\n", cur_min, i);
			tpi = pm_arg->cur_pos_item[i];
			score += BM25_term_i_score(me_arg->bm25args, i, tpi->tf, doclen);
			hit_terms ++;
		}

	rank_cram(me_arg->rk_set, docID, score, hit_terms,
	          (char**)pm_arg->posting_args, NULL);
	//printf("(BM25 score = %f)\n", score);
}

char *get_doc_path(keyval_db_t kv_db, doc_id_t docID)
{
	char *doc_path;
	size_t val_sz;

	if(NULL == (doc_path = keyval_db_get(kv_db, &docID, sizeof(doc_id_t),
	                                     &val_sz))) {
		printf("get error: %s\n", keyval_db_last_err(kv_db));
		return NULL;
	}

	return doc_path;
}

void print_res_snippet(doc_id_t docID, char **terms, uint32_t n_terms,
                       keyval_db_t kv_db)
{
	uint32_t i;
	size_t val_sz;
	docterm_t docterm = {docID, ""};
	docterm_pos_t *termpos;
	char *doc_path;

	FILE *doc_fh;
	list  snippet_div_list = LIST_NULL;

	doc_path = get_doc_path(kv_db, docID);
	printf("@ %s\n", doc_path);

	for (i = 0; i < n_terms; i++) {
		strcpy(docterm.term, terms[i]);

		termpos = keyval_db_get(kv_db, &docterm,
								sizeof(doc_id_t) + strlen(docterm.term),
								&val_sz);
		if(NULL != termpos) {
			//printf("`%s' ", docterm.term);
			//printf("pos = %u[%u] ", termpos->doc_pos, termpos->n_bytes);
			snippet_add_pos(&snippet_div_list, docterm.term,
			                termpos->doc_pos, termpos->n_bytes);
			free(termpos);
		}
	}
	//printf("\n");

	doc_fh = fopen(doc_path, "r");
	if (doc_fh) {
		snippet_read_file(doc_fh, &snippet_div_list);
		fclose(doc_fh);

		snippet_hi_print(&snippet_div_list);
	}

	snippet_free_div_list(&snippet_div_list);

	free(doc_path);
}

void print_res_item(struct rank_set *rs, struct rank_item* ri,
                    uint32_t cnt, void*arg)
{
	P_CAST(kv_db, keyval_db_t, arg);

	printf("result#%u: doc#%u score=%.3f\n", cnt, ri->docID, ri->score);
	print_res_snippet(ri->docID, ri->terms, rs->n_terms, kv_db);
}

uint32_t print_all_rank_res(struct rank_set *rs, keyval_db_t keyval_db)
{
	struct rank_wind win;
	uint32_t         total_pages, page = 0;
	const uint32_t   res_per_page = DEFAULT_RES_PER_PAGE;

	do {
		win = rank_window_calc(rs, page, res_per_page, &total_pages);
		if (win.to > 0) {
			printf("page#%u (from %u to %u):\n",
			       page + 1, win.from, win.to);
			rank_window_foreach(&win, &print_res_item, keyval_db);
			page ++;
		}
	} while (page < total_pages);

	return total_pages;
}

static void
do_term_search(void *ti, keyval_db_t keyval_db, enum postmerge_op op,
               char (*terms)[MAX_TERM_BYTES], uint32_t n_terms)
{
	int                     i;
	void                   *posting;
	term_id_t               term_id;
	struct merge_extra_arg  me_arg;
	struct rank_set         rk_set;
	uint32_t                res_pages;
	uint32_t                docN, df;
	struct BM25_term_i_args bm25args;
	struct postmerge_arg    pm_arg;

	postmerge_posts_clear(&pm_arg);
	pm_arg.op = op;

	/*
	 * for each term posting list, pre-calculate some scoring
	 * parameters and find associated posting list.
	 */
	docN = term_index_get_docN(ti);

	for (i = 0; i < n_terms; i++) {
		term_id = term_lookup(ti, terms[i]);
		if (term_id != 0) {
			posting = term_index_get_posting(ti, term_id);

			df = term_index_get_df(ti, term_id);
			bm25args.idf[i] = BM25_idf((float)df, (float)docN);

			printf("term `%s' ID = %u, df[%d] = %u.\n",
			       terms[i], term_id, i, df);
		} else {
			posting = NULL;

			df = 0;
			bm25args.idf[i] = BM25_idf((float)df, (float)docN);

			printf("term `%s' not found, df[%d] = %u.\n",
			       terms[i], i, df);
		}

		postmerge_posts_add(&pm_arg, posting, terms[i]);
	}

	/*
	 * prepare some scoring parameters.
	 */
	bm25args.n_postings = i;
	bm25args.avgDocLen = (float)term_index_get_avgDocLen(ti);
	bm25args.b  = BM25_DEFAULT_B;
	bm25args.k1 = BM25_DEFAULT_K1;
	bm25args.frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25args.avgDocLen;

	printf("BM25 arguments:\n");
	BM25_term_i_args_print(&bm25args);

	/*
	 * prepare term posting list merge callbacks.
	 */
	pm_arg.post_start_fun = &term_posting_start;
	pm_arg.post_finish_fun = &term_posting_finish;
	pm_arg.post_jump_fun = &term_posting_jump_wrap;
	pm_arg.post_next_fun = &term_posting_next;
	pm_arg.post_now_fun = &term_posting_current_wrap;
	pm_arg.post_now_id_fun = &term_posting_current_id_wrap;
	pm_arg.post_on_merge = &term_posting_on_merge;

	/*
	 * initialize ranking set given number of terms.
	 */
	rank_init(&rk_set, RANK_SET_DEFAULT_SZ, n_terms, 0);

	/*
	 * merge extra arguments.
	 */
	me_arg.term_index = ti;
	me_arg.bm25args = &bm25args;
	me_arg.keyval_db = keyval_db;
	me_arg.rk_set = &rk_set;

	/*
	 * merge and score.
	 */
	printf("start merging...\n");
	if (!posting_merge(&pm_arg, &me_arg))
		fprintf(stderr, "posting merge operation undefined.\n");

	/*
	 * rank top K hits.
	 */
	rank_sort(&rk_set);

	/*
	 * print ranked search results by page number.
	 */
	res_pages = print_all_rank_res(&rk_set, keyval_db);
	printf("result(s): %u pages.\n", res_pages);

	rank_uninit(&rk_set);
}

void
math_posting_on_merge(uint64_t cur_min, struct postmerge_arg* pm_arg,
                      void* extra_args)
{
	uint32_t i;
	struct math_posting_item* po_item;

	for (i = 0; i < pm_arg->n_postings; i++) {
		po_item = pm_arg->cur_pos_item[i];
		printf("merge docID#%u expID#%u from posting[%u]\n",
		       po_item->doc_id, po_item->exp_id, i);
	}
}

enum dir_merge_ret
on_dir_merge(math_posting_t postings[MAX_MATH_PATHS], uint32_t n_postings,
             uint32_t level, void *args)
{
	P_CAST(pm_arg, struct postmerge_arg, args);

	uint32_t i;
	math_posting_t po;
	struct subpath_ele *ele;
//	uint32_t j;
//	struct subpath *sp;
//	const char *fullpath;

	postmerge_posts_clear(pm_arg);
//	printf("====\n");
	for (i = 0; i < n_postings; i++) {
		po = postings[i];

		ele = math_posting_get_ele(po);
		postmerge_posts_add(pm_arg, po, ele);

//		fullpath = math_posting_get_pathstr(po);
//
//		printf("posting[%u]: %s ", i, fullpath);
//
//		printf("(duplicates: ");
//		for (j = 0; j <= ele->dup_cnt; j++) {
//			sp = ele->dup[j];
//			printf("path#%u ", sp->path_id);
//		}
//		printf(")\n");
	}
//	printf("~~~~\n");

	if (!posting_merge(pm_arg, NULL))
		fprintf(stderr, "math posting merge failed.");

	return DIR_MERGE_RET_CONTINUE;
}

static void
do_math_search(math_index_t mi, char *tex)
{
	struct tex_parse_ret parse_ret;
	struct postmerge_arg pm_arg;

	/*
	 * prepare term posting list merge callbacks.
	 */
	pm_arg.post_start_fun = &math_posting_start;
	pm_arg.post_finish_fun = &math_posting_finish;
	pm_arg.post_jump_fun = &math_posting_jump;
	pm_arg.post_next_fun = &math_posting_next;
	pm_arg.post_now_fun = &math_posting_current_wrap;
	pm_arg.post_now_id_fun = &math_posting_current_id_wrap;
	pm_arg.post_on_merge = &math_posting_on_merge;

	/* overwrites to AND merge */
	pm_arg.op = POSTMERGE_OP_AND;

	/* parse TeX */
	parse_ret = tex_parse(tex, 0, false);

	if (parse_ret.code == PARSER_RETCODE_SUCC) {
		subpaths_print(&parse_ret.subpaths, stdout);

		printf("calling math_index_dir_merge()...\n");
		math_index_dir_merge(mi, DIR_MERGE_DEPTH_FIRST,
		                     &parse_ret.subpaths, &on_dir_merge, &pm_arg);

		subpaths_release(&parse_ret.subpaths);
	} else {
		printf("parser error: %s\n", parse_ret.msg);
	}
}

int main(int argc, char *argv[])
{
	int                     opt, i;
	void                   *ti = NULL;
	math_index_t            mi = NULL;
	keyval_db_t             keyval_db = NULL;
	enum postmerge_op       op;

	char       query[MAX_MERGE_POSTINGS][MAX_QUERY_BYTES];
	uint32_t   n_queries = 0;

	char      *index_path = NULL;
	const char kv_db_fname[] = "kvdb-offset.bin";
	char       term_index_path[MAX_DIR_PATH_NAME_LEN];

	while ((opt = getopt(argc, argv, "hp:t:o:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("test for merge postings.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <index path> | -t <term> | -o <op>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp -t 'nick ' -t 'wilde' -o OR\n", argv[0]);
			printf("%s -p ./tmp -t 'give ' -t 'up' -t 'dream' -o AND\n", argv[0]);
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			strcpy(query[n_queries ++], optarg);
			break;

		case 'o':
		{
			if (strcmp(optarg, "AND") == 0)
				op = POSTMERGE_OP_AND;
			else if (strcmp(optarg, "OR") == 0)
				op = POSTMERGE_OP_OR;
			else
				op = POSTMERGE_OP_UNDEF;

			break;
		}

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * check program arguments.
	 */
	if (index_path == NULL || n_queries == 0) {
		printf("not enough arguments.\n");
		goto exit;
	}

	/*
	 * print program arguments.
	 */
	printf("index path: %s\n", index_path);
	printf("query: ");
	for (i = 0; i < n_queries; i++) {
		printf("`%s'", query[i]);
		if (i + 1 != n_queries)
			printf(", ");
		else
			printf(".");
	}
	printf("\n");

	/*
	 * open term index.
	 */
	printf("opening term index ...\n");
	sprintf(term_index_path, "%s/term", index_path);
	mkdir_p(term_index_path);
	ti = term_index_open(term_index_path, TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
		printf("cannot create/open term index.\n");
		goto exit;
	}

	/*
	 * open math index.
	 */
	printf("opening math index ...\n");
	mi = math_index_open(index_path, MATH_INDEX_READ_ONLY);
	if (mi == NULL) {
		printf("cannot create/open math index.\n");
		goto exit;
	}

	/*
	 * open document offset key-value database.
	 */
	printf("opening document offset key-value DB...\n");
	keyval_db = keyval_db_open_under(kv_db_fname, index_path,
	                                 KEYVAL_DB_OPEN_RD);
	if (keyval_db == NULL) {
		printf("key-value DB open error.\n");
		goto exit;
	} else {
		printf("%lu records in key-value DB.\n",
		       keyval_db_records(keyval_db));
	}

	if (n_queries == 1) {
		do_math_search(mi, query[0]);
	} else {
		do_term_search(ti, keyval_db, op, query, n_queries);
	}

exit:
	if (index_path)
		free(index_path);

	if (ti) {
		printf("closing term index...\n");
		term_index_close(ti);
	}

	if (mi) {
		printf("closing math index...\n");
		math_index_close(mi);
	}

	if (keyval_db) {
		printf("closing key-value DB...\n");
		keyval_db_close(keyval_db);
	}

	return 0;
}
