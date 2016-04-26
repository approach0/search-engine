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

#include "txt-seg/txt-seg.h"
#include "txt-seg/config.h"
#include "indexer/doc-term-pos.h"

struct merge_score_arg {
	void                    *term_index;
	struct BM25_term_i_args *bm25args;
	keyval_db_t              keyval_db;
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
	uint32_t i;
	float score = 0.f;
	doc_id_t docID = cur_min;
	char *doc_path;
	size_t val_sz;

	docterm_t docterm = {docID, ""};
	docterm_pos_t *termpos;

	P_CAST(ms_arg, struct merge_score_arg, extra_args);
	float doclen = (float)term_index_get_docLen(ms_arg->term_index, docID);
	struct term_posting_item *tpi;

	for (i = 0; i < pm_arg->n_postings; i++)
		if (pm_arg->curIDs[i] == cur_min) {
			printf("merge docID#%lu from posting[%d] ", cur_min, i);
			tpi = pm_arg->cur_pos_item[i];
			score += BM25_term_i_score(ms_arg->bm25args, i, tpi->tf, doclen);

			strcpy(docterm.term, pm_arg->posting_args[i]);
			printf("(term `%s');", docterm.term);
			
			termpos = keyval_db_get(ms_arg->keyval_db, &docterm,
			                        sizeof(doc_id_t) + strlen(docterm.term),
			                        &val_sz);
			if(NULL != termpos) {
				printf("term `%s' ", docterm.term);
				printf("pos = %u[%u]", termpos->doc_pos, termpos->n_bytes);
				//FILE *fh = fopen("", "r");
				free(termpos);
			} else {
				printf("get error: %s", keyval_db_last_err(ms_arg->keyval_db));
			}

			printf("\n");
		}

	if(NULL != (doc_path = keyval_db_get(ms_arg->keyval_db, &docID,
	                                     sizeof(doc_id_t), &val_sz))) {
		printf("`%s' ", doc_path);
		free(doc_path);
	} else {
		printf("get error: %s\n", keyval_db_last_err(ms_arg->keyval_db));
	}

	printf("(BM25 score = %f)\n", score);
}

int main(int argc, char *argv[])
{
	int                     i, opt;
	void                   *ti, *posting;
	term_id_t               term_id;
	struct postmerge_arg    pm_arg;
	struct merge_score_arg  ms_arg;
	keyval_db_t             keyval_db;

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

	pm_arg.extra_args = &bm25args;
	pm_arg.post_start_fun = &term_posting_start_wrap;
	pm_arg.post_finish_fun = &term_posting_finish_wrap;
	pm_arg.post_jump_fun = &term_posting_jump_wrap;
	pm_arg.post_next_fun = &term_posting_next;
	pm_arg.post_now_fun = &term_posting_current_wrap;
	pm_arg.post_now_id_fun = &term_posting_current_id_wrap;
	pm_arg.post_on_merge = &posting_on_merge;

	ms_arg.term_index = ti;
	ms_arg.bm25args = &bm25args;
	ms_arg.keyval_db = keyval_db;

	printf("start merging...\n");
	if (!posting_merge(&pm_arg, &ms_arg))
		fprintf(stderr, "posting merge operation undefined.\n");

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
