#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>

#include "mhook/mhook.h"
#include "postmerge.h"
#include "math-expr-search.h"

static void
math_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	struct math_posting_item_v2   *po_item;
	//P_CAST(mes_arg, struct math_extra_score_arg, extra_args);

	for (int i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			po_item = pm->cur_pos_item[i];
			printf("doc#%u, exp#%u\n", po_item->doc_id, po_item->exp_id);
			printf("lr_paths=%u, paths=%u, @pos%u\n", po_item->n_lr_paths,
			        po_item->n_paths, po_item->pathinfo_pos);
		}
	}

	/* calculate math similarity on merge */
	//res = math_expr_score_on_merge(pm, mes_arg->dir_merge_level,
	//                              mes_arg->n_qry_lr_paths);

	// if (res.score > 0.f) {
	// 	printf("docID#%u expID#%u score: %u\n",
	// 	       res.doc_id, res.exp_id, res.score);
	// }
}

int main(int argc, char *argv[])
{
	uint32_t                i;
	int                     opt;
	math_index_t            mi = NULL;
	enum dir_merge_type     directory_merge_method = DIR_MERGE_DIRECT;

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
			           "-m <1:dfs,2:bfs,3:direct>"
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
				directory_merge_method = DIR_MERGE_BREADTH_FIRST;
			else if (optarg[0] == '2')
				directory_merge_method = DIR_MERGE_DEPTH_FIRST;
			else
				directory_merge_method = DIR_MERGE_DIRECT;

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
	math_expr_search(mi, query[0], directory_merge_method,
	                 &math_posting_on_merge, NULL);

exit:
	free(index_path);

	if (mi) {
		printf("closing math index...\n");
		math_index_close(mi);
	}

	mhook_print_unfree();
	return 0;
}
