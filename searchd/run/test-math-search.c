#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "search.h"

static void
math_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	struct math_score_res res;

	/* get additional math score arguments */
	P_CAST(mes_arg, struct math_extra_score_arg, extra_args);

	/* calculate math similarity on merge */
	res = math_score_on_merge(pm, mes_arg->dir_merge_level,
	                          mes_arg->n_qry_lr_paths);

	if (res.score > 0.f) {
		printf("docID#%u expID#%u score: %u\n",
		       res.doc_id, res.exp_id, res.score);
	}
}

int main(int argc, char *argv[])
{
	uint32_t                i;
	int                     opt;
	math_index_t            mi = NULL;
	keyval_db_t             keyval_db = NULL;

	char       query[MAX_MERGE_POSTINGS][MAX_QUERY_BYTES];
	uint32_t   n_queries = 0;

	char      *index_path = NULL;
	const char kv_db_fname[] = "kvdb-offset.bin";

	while ((opt = getopt(argc, argv, "hp:t:o:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("test for math search.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <index path> | -t <TeX>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp -t 'a+b'\n", argv[0]);
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			strcpy(query[n_queries ++], optarg);
			break;

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

	/* for now only single (query[0]) is searched */
	math_search_posting_merge(mi, query[0], DIR_MERGE_DEPTH_FIRST,
	                          &math_posting_on_merge, NULL);

exit:
	if (index_path)
		free(index_path);

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
