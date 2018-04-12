#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "mhook/mhook.h"

#include "tex-parser/tex-parser.h"
#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"
#include "tex-parser/gen-symbol.h"
#include "tex-parser/trans.h"

#include "postmerge.h"
#include "math-expr-search.h"
#include "math-prefix-qry.h"

static int
math_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	struct math_expr_score_res res;
	P_CAST(mesa, struct math_extra_score_arg, extra_args);

	res = math_expr_prefix_score_on_merge(cur_min, pm, mesa->n_qry_lr_paths,
	                                      &mesa->pq, mesa->dir_merge_level, NULL);
	printf("doc#%u, exp#%u score: %u \n", res.doc_id, res.exp_id, res.score);
	return 0;
}

int main(int argc, char *argv[])
{
	uint32_t                i;
	int                     opt;
	math_index_t            mi = NULL;
	enum math_expr_search_policy srch_policy = MATH_SRCH_SUBEXPRESSION;

	static char query[MAX_MERGE_POSTINGS][MAX_QUERY_BYTES];
	uint32_t   n_queries = 0;

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

	/* for now only single (query[0]) is searched */
	math_expr_search(mi, query[0], srch_policy, &math_posting_on_merge, NULL);

exit:
	free(index_path);

	if (mi) {
		printf("closing math index...\n");
		math_index_close(mi);
	}

	mhook_print_unfree();
	return 0;
}
