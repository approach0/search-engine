#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "indices/indices.h"

#include "config.h"
#include "math-expr-search.h"
#include "search-utils.h"

int main(int argc, char *argv[])
{
	int            opt;
	struct indices indices;
	ranked_results_t rk_res;
	enum math_expr_search_policy srch_policy = MATH_SRCH_FUZZY_STRUCT;

	static char query[MAX_QUERY_BYTES] = {0};

	char      *index_path = NULL;

	while ((opt = getopt(argc, argv, "hp:t:o:m:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("test for math search.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <index path> | -t <TeX> | "
			           "-m <1:exact,2:subexpr,3:fuzzy>"
			           "\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp -t 'a+b'\n", argv[0]);
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 'm':
			if (optarg[0] == '1')
				srch_policy = MATH_SRCH_EXACT_STRUCT;
			else if (optarg[0] == '2')
				srch_policy = MATH_SRCH_SUBEXPRESSION;
			else
				srch_policy = MATH_SRCH_FUZZY_STRUCT;

			break;

		case 't':
			strcpy(query, optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * check program arguments.
	 */
	if (index_path == NULL || query[0] == '\0') {
		printf("not enough arguments.\n");
		goto exit;
	}

	/*
	 * Open indices.
	 */
	printf("index path: %s\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	printf("caching math index...\n");
	postlist_cache_set_limit(&indices.ci, 7 KB, 8 KB);
	indices_cache(&indices);

	size_t n = math_postlist_cache_list(indices.ci.math_cache, 1);
	printf("cached items: %lu, total size: %lu \n", n,
	       indices.ci.math_cache.postlist_sz);

	printf("searching query...\n");
	rk_res = math_expr_search(&indices, query, srch_policy);
	print_search_results(&rk_res, 1, &indices); /* print page 1 only */
	free_ranked_results(&rk_res);

close:
	indices_close(&indices);

exit:
	free(index_path);
	mhook_print_unfree();
	return 0;
}
