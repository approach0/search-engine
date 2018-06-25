#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "common/common.h"
#include "mhook/mhook.h"

#include "tex-parser/tex-parser.h"
#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"
#include "tex-parser/gen-symbol.h"
#include "tex-parser/trans.h"

#include "postmerge/postmerge.h"
#include "indices/indices.h"

#include "config.h"
#include "math-expr-search.h"
#include "math-prefix-qry.h"

static int
on_merge(uint64_t cur_min, struct postmerge* pm, void* args)
{
	PTR_CAST(mesa, struct math_extra_score_arg, args);
	struct indices *indices = mesa->expr_srch_arg;

	/* printing */
//	for (u32 i = 0; i < pm->n_postings; i++) {
//		PTR_CAST(mepa, struct math_extra_posting_arg, pm->posting_args[i]);
//		printf("%s ", mepa->base_path);
//		subpath_set_print_ele(mepa->ele);
//		if (mepa->type == MATH_POSTLIST_TYPE_MEMORY)
//			printf(" (in-memory)");
//		printf("\n");
//
//		if (pm->curIDs[i] == cur_min) {
//			PTR_CAST(item, struct math_posting_compound_item_v2,
//			         pm->cur_pos_item[i]);
//			printf("doc#%u, exp#%u.  n_paths: %u, n_lr_paths: %u \n",
//			       item->doc_id, item->exp_id,
//			       item->n_paths, item->n_lr_paths);
//		}
//	}
	
	/* score calculation */
	struct math_expr_score_res res = {0};
	res = math_expr_prefix_score_on_merge(cur_min, pm, mesa, indices);
	printf("doc#%u, exp#%u, score: %u\n",
	       res.doc_id, res.exp_id, res.score);
	return 0;
}

int main(int argc, char *argv[])
{
	int            opt;
	struct indices indices;
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
	math_expr_search(&indices, query, srch_policy, &on_merge, &indices);

close:
	indices_close(&indices);

exit:
	free(index_path);
	mhook_print_unfree();
	return 0;
}
