#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "mhook/mhook.h"
#include "common/common.h"

#include "indexer/index.h" /* required by query.h */
#include "query.h"
#include "search.h"

int main(int argc, char *argv[])
{
	struct query         qry;
	struct indices       indices;
	int                  opt;
	char                 index_path[1024];
	ranked_results_t     results;

	qry = query_new();

	while ((opt = getopt(argc, argv, "hi:m:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("testcase of search function.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -i <index path> |"
			       " -m <tex> |"
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'i':
			strcpy(index_path, optarg);
			break;

		case 'm':
			query_push_keyword_str(&qry, optarg, QUERY_KEYWORD_TEX);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (index_path == NULL || qry.len == 0) {
		printf("not enough arguments.\n");
		goto exit;
	}

	printf("opening index at path: `%s' ...\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	printf("caching math index...\n");
	postlist_cache_set_limit(&indices.ci, 7 KB, 8 KB);
	indices_cache(&indices);

	/* search query */
	results = indices_run_query(&indices, &qry);

	/* print ranked search results in pages */
	//print_res(&results, page, &args);

	/* free ranked results */
	free_ranked_results(&results);

close:
	printf("closing index...\n");
	indices_close(&indices);

exit:
	printf("existing...\n");
	query_delete(qry);

	mhook_print_unfree();
	return 0;
}
