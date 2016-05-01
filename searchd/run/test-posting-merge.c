#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>

#include "list/list.h"
#include "term-index/term-index.h"
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

void term_posting_start_wrap(void *posting)
{
	if (posting)
		term_posting_start(posting);
}

void term_posting_finish_wrap(void *posting)
{
	if (posting)
		term_posting_finish(posting);
}

void *term_posting_current_wrap(void *posting)
{
	if (posting)
		return term_posting_current(posting);
	else
		return NULL;
}

uint64_t term_posting_current_id_wrap(void *item)
{
	doc_id_t doc_id;
	if (item) {
		doc_id = ((struct term_posting_item*)item)->doc_id;
		return (uint64_t)doc_id;
	} else {
		return MAX_POST_ITEM_ID;
	}
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

void posting_on_merge(uint64_t cur_min, struct postmerge_arg* pm_arg, void* extra_args)
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

	printf("#%u: %.3f doc#%u\n", cnt, ri->score, ri->docID);
	print_res_snippet(ri->docID, ri->terms, rs->n_terms, kv_db);
}

void print_all_rank_res(struct rank_set *rs, keyval_db_t keyval_db)
{
	struct rank_wind win;
	uint32_t         total_pages, page = 0;
	const uint32_t   res_per_page = DEFAULT_RES_PER_PAGE;

	do {
		printf("page#%u:\n", page + 1);
		win = rank_window_calc(rs, page, res_per_page, &total_pages);
		rank_window_foreach(&win, &print_res_item, keyval_db);
		page ++;
	} while (page < total_pages);
}

int main(int argc, char *argv[])
{
	int                     i, opt;
	void                   *ti, *posting;
	term_id_t               term_id;
	struct postmerge_arg    pm_arg;
	struct merge_extra_arg  me_arg;
	keyval_db_t             keyval_db;
	struct rank_set         rk_set;

	char    *index_path = NULL;
	char    *terms[MAX_MERGE_POSTINGS];
	char    *keyval_db_path;
	uint32_t docN, df, n_terms = 0;

	struct BM25_term_i_args bm25args;

	/* initially, set pm_arg.op to undefined */
	postmerge_arg_init(&pm_arg);

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
			goto free_args;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			terms[n_terms ++] = strdup(optarg);
			break;

		case 'o':
		{
			if (strcmp(optarg, "AND") == 0)
				pm_arg.op = POSTMERGE_OP_AND;
			else if (strcmp(optarg, "OR") == 0)
				pm_arg.op = POSTMERGE_OP_OR;
			else
				pm_arg.op = POSTMERGE_OP_UNDEF;

			break;
		}

		default:
			printf("bad argument(s). \n");
			goto free_args;
		}
	}

	if (index_path == NULL || n_terms == 0) {
		printf("not enough arguments.\n");
		goto free_args;
	}

	printf("index path: %s\n", index_path);
	printf("terms: ");
	for (i = 0; i < n_terms; i++) {
		printf("`%s'", terms[i]);
		if (i + 1 != n_terms)
			printf(", ");
		else
			printf(".");
	}
	printf("\n");

	ti = term_index_open(index_path, TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
		printf("index not found.\n");
		goto free_args;
	}

	keyval_db_path = (char *)malloc(1024);
	strcpy(keyval_db_path, index_path);
	/* trim trailing slash (user may specify path ending with a slash) */
	if (keyval_db_path[strlen(keyval_db_path) - 1] == '/')
		keyval_db_path[strlen(keyval_db_path) - 1] = '\0';
	strcat(keyval_db_path, "/kv-termpos.bin");
	printf("opening key-value DB (%s)...\n", keyval_db_path);
	keyval_db = keyval_db_open(keyval_db_path, KEYVAL_DB_OPEN_RD);
	free(keyval_db_path);

	if (keyval_db == NULL) {
		printf("key-value DB open error.\n");
		goto key_value_db_fails;
	} else {
		printf("%lu records in key-value DB.\n",
		       keyval_db_records(keyval_db));
	}

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

		postmerge_arg_add_post(&pm_arg, posting, terms[i]);
	}

	bm25args.n_postings = i;
	bm25args.avgDocLen = (float)term_index_get_avgDocLen(ti);
	bm25args.b  = BM25_DEFAULT_B;
	bm25args.k1 = BM25_DEFAULT_K1;
	bm25args.frac_b_avgDocLen = BM25_DEFAULT_K1 / bm25args.avgDocLen;

	printf("BM25 arguments:\n");
	BM25_term_i_args_print(&bm25args);

	pm_arg.post_start_fun = &term_posting_start_wrap;
	pm_arg.post_finish_fun = &term_posting_finish_wrap;
	pm_arg.post_jump_fun = &term_posting_jump_wrap;
	pm_arg.post_next_fun = &term_posting_next;
	pm_arg.post_now_fun = &term_posting_current_wrap;
	pm_arg.post_now_id_fun = &term_posting_current_id_wrap;
	pm_arg.post_on_merge = &posting_on_merge;

	/* initialize ranking set given number of terms */
	rank_init(&rk_set, RANK_SET_DEFAULT_SZ, n_terms, 0);

	me_arg.term_index = ti;
	me_arg.bm25args = &bm25args;
	me_arg.keyval_db = keyval_db;
	me_arg.rk_set = &rk_set;

	printf("start merging...\n");
	if (!posting_merge(&pm_arg, &me_arg))
		fprintf(stderr, "posting merge operation undefined.\n");

	rank_sort(&rk_set);
	print_all_rank_res(&rk_set, keyval_db);
	rank_uninit(&rk_set);

key_value_db_fails:
	printf("closing term index...\n");
	term_index_close(ti);

free_args:
	printf("free arguments...\n");
	for (i = 0; i < n_terms; i++)
		free(terms[i]);

	if (index_path)
		free(index_path);

	return 0;
}
