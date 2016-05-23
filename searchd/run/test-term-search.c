#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>

#include "list/list.h"
#include "term-index/term-index.h"
#include "keyval-db/keyval-db.h"
#include "dir-util/dir-util.h"

#include "postmerge.h"
#include "bm25-score.h"
#include "rank.h"
#include "snippet.h"
#include "mem-posting.h"

#include "txt-seg/txt-seg.h"
#include "txt-seg/config.h"
#include "indexer/doc-term-pos.h"

struct term_extra_score_arg {
	void                    *term_index;
	struct BM25_term_i_args *bm25args;
	struct rank_set         *rk_set;
};

void *term_posting_current_wrap(void *posting)
{
	return (void*)term_posting_current(posting);
}

uint64_t term_posting_current_id_wrap(void *item)
{
	doc_id_t doc_id;
	doc_id = ((struct term_posting_item*)item)->doc_id;
	return (uint64_t)doc_id;
}

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
term_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	uint32_t i, hit_terms = 0;
	float score = 0.f;
	doc_id_t docID = cur_min;

	P_CAST(tes_arg, struct term_extra_score_arg, extra_args);
	float doclen = (float)term_index_get_docLen(tes_arg->term_index, docID);
	struct term_posting_item *tpi;

	for (i = 0; i < pm->n_postings; i++)
		if (pm->curIDs[i] == cur_min) {
			//printf("merge docID#%lu from posting[%d]\n", cur_min, i);
			tpi = pm->cur_pos_item[i];
			score += BM25_term_i_score(tes_arg->bm25args, i, tpi->tf, doclen);
			hit_terms ++;
		}

	rank_cram(tes_arg->rk_set, docID, score, hit_terms,
	          (char**)pm->posting_args, NULL);
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

struct mem_posting *term_posting_fork(void *term_posting)
{
	struct term_posting_item *pi;
	struct mem_posting *ret_mempost, *buf_mempost;

	const struct codec codecs[] = {
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL}
	};

	/* create memory posting to be encoded */
	ret_mempost = mem_posting_create(DEFAULT_SKIPPY_SPANS);
	mem_posting_set_enc(ret_mempost, sizeof(struct term_posting_item),
	                    codecs, sizeof(codecs));

	/* create a temporary memory posting */
	buf_mempost = mem_posting_create(MAX_SKIPPY_SPANS);

	/* start iterating term posting list */
	term_posting_start(term_posting);

	do {
		pi = term_posting_current(term_posting);
		mem_posting_write(buf_mempost, 0, pi,
		                  sizeof(struct term_posting_item));

	} while (term_posting_next(term_posting));

	/* finish iterating term posting list */
	term_posting_finish(term_posting);

	/* encode */
	mem_posting_encode(ret_mempost, buf_mempost);
	mem_posting_release(buf_mempost);
	return ret_mempost;
}

static void
do_term_search(void *ti, keyval_db_t keyval_db, enum postmerge_op op,
               char (*terms)[MAX_TERM_BYTES], uint32_t n_terms)
{
	int                          i;
	void                        *posting;
	term_id_t                    term_id;
	struct term_extra_score_arg  tes_arg;
	struct rank_set              rk_set;
	uint32_t                     res_pages;
	uint32_t                     docN, df;
	struct BM25_term_i_args      bm25args;
	struct postmerge             pm;
	struct postmerge_callbks     mem_calls, disk_calls, *calls = NULL;

////////////
struct mem_posting *fork_posting;
void *term_posting;

#define WORD "posting"

term_id = term_lookup(ti, WORD);
term_posting = term_index_get_posting(ti, term_id);
fork_posting = term_posting_fork(term_posting);
printf("forked posting list: ");
mem_posting_print_meminfo(fork_posting);
/////////////

	postmerge_posts_clear(&pm);

	/*
	 * prepare term posting list merge callbacks.
	 */
	mem_calls.start  = &term_posting_start;
	mem_calls.finish = &term_posting_finish;
	mem_calls.jump   = &term_posting_jump_wrap;
	mem_calls.next   = &term_posting_next;
	mem_calls.now    = &term_posting_current_wrap;
	mem_calls.now_id = &term_posting_current_id_wrap;

	disk_calls.start  = &mem_posting_start;
	disk_calls.finish = &mem_posting_finish;
	disk_calls.jump   = &mem_posting_jump;
	disk_calls.next   = &mem_posting_next;
	disk_calls.now    = &mem_posting_current;
	disk_calls.now_id = &mem_posting_current_id;

	/*
	 * for each term posting list, pre-calculate some scoring
	 * parameters and find associated posting list.
	 */
	docN = term_index_get_docN(ti);

	for (i = 0; i < n_terms; i++) {
		term_id = term_lookup(ti, terms[i]);
		if (term_id != 0) {

			if (0 == strcmp(WORD, terms[i])) {
				posting = fork_posting;
				calls = &disk_calls;
			} else {
				posting = term_index_get_posting(ti, term_id);
				calls = &mem_calls;
			}

			df = term_index_get_df(ti, term_id);
			bm25args.idf[i] = BM25_idf((float)df, (float)docN);

			printf("term `%s' ID = %u, df[%d] = %u.\n",
			       terms[i], term_id, i, df);
		} else {
			posting = NULL;
			calls = NULL;

			df = 0;
			bm25args.idf[i] = BM25_idf((float)df, (float)docN);

			printf("term `%s' not found, df[%d] = %u.\n",
			       terms[i], i, df);
		}

		postmerge_posts_add(&pm, posting, calls, terms[i]);
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
	 * initialize ranking set given number of terms.
	 */
	rank_init(&rk_set, RANK_SET_DEFAULT_SZ, n_terms, 0);

	/*
	 * merge extra arguments.
	 */
	tes_arg.term_index = ti;
	tes_arg.bm25args = &bm25args;
	tes_arg.rk_set = &rk_set;

	/*
	 * merge and score.
	 */
	printf("start merging...\n");

	printf("Press Enter to Continue");
	while( getchar() != '\n' );

	if (!posting_merge(&pm, op, &term_posting_on_merge, &tes_arg))
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

////////////
mem_posting_release(fork_posting);
/////////////
}

int main(int argc, char *argv[])
{
	int                     opt, i;
	void                   *ti = NULL;
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

	do_term_search(ti, keyval_db, op, query, n_queries);

exit:
	if (index_path)
		free(index_path);

	if (ti) {
		printf("closing term index...\n");
		term_index_close(ti);
	}

	if (keyval_db) {
		printf("closing key-value DB...\n");
		keyval_db_close(keyval_db);
	}

	return 0;
}
