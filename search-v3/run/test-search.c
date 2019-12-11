#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "txt-seg/txt-seg.h"
#include "search.h"
#include "query.h"
#include "print-search-results.h"

int main(int argc, char *argv[])
{
	struct indices indices;
	char indices_path[1024] = "./tmp";
	struct query qry = QUERY_NEW;
	int page = 1;
	size_t ti_cache_limit = 0, mi_cache_limit = 0;

	int opt;
	while ((opt = getopt(argc, argv, "hi:d:t:m:p:c:C:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("test search functions.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s \n"
			       " -h (show this help text) \n"
			       " -i <index path> (default: %s) \n"
			       " -d <dict path> (should be passed before -t) \n"
			       " -t <text keyword> \n"
			       " -m <math keyword> \n"
			       " -p <page> (0 for all pages) \n"
			       " -c <term cache size (MB)> \n"
			       " -C <math cache size (MB)> \n"
			       "\n", argv[0], indices_path);
			return 0;

		case 'i':
			strcpy(indices_path, optarg);
			break;

		case 'd':
			printf("open segment dictionary at [%s]\n", optarg);
			text_segment_init(optarg);
			break;

		case 't':
			query_digest_txt(&qry, optarg);
			break;

		case 'm':
			query_push_kw(&qry, optarg, QUERY_KW_TEX, QUERY_OP_OR);
			break;

		case 'p':
			sscanf(optarg, "%d", &page);
			break;

		case 'c':
			sscanf(optarg, "%lu", &ti_cache_limit);
			break;

		case 'C':
			sscanf(optarg, "%lu", &mi_cache_limit);
			break;

		default:
			printf("bad argument(s). \n");
			return 1;
		}
	}

	/* open indices */
	if(indices_open(&indices, indices_path, INDICES_OPEN_RD)) {
		prerr("indices open failed: %s", indices_path);
		goto close;
	}

	/* cache index */
	indices.ti_cache_limit = ti_cache_limit;
	indices.mi_cache_limit = mi_cache_limit;
	indices_cache(&indices);
	indices_print_summary(&indices);
	printf("\n");

	/* make up a query */
	if (qry.len == 0) {
		query_push_kw(&qry, "prove", QUERY_KW_TERM, QUERY_OP_AND);
		query_push_kw(&qry, "x^2+y^2=z^2", QUERY_KW_TEX, QUERY_OP_OR);
		query_push_kw(&qry, "x^^" /* malformed */, QUERY_KW_TEX, QUERY_OP_OR);
		query_push_kw(&qry, "a^2+b^2", QUERY_KW_TEX, QUERY_OP_OR);
		query_push_kw(&qry, "Pythagoras", QUERY_KW_TERM, QUERY_OP_OR);
		query_push_kw(&qry, "theorem", QUERY_KW_TERM, QUERY_OP_OR);
		query_push_kw(&qry, "Kunming", QUERY_KW_TERM, QUERY_OP_NOT);
		query_push_kw(&qry, "pythagoras", QUERY_KW_TERM, QUERY_OP_OR);
	}

	query_print(qry, stdout);
	printf("\n");

	/* perform searching and print results */
	ranked_results_t rk_res = indices_run_query(&indices, &qry, NULL, 0, NULL);
	print_search_results(&indices, &rk_res, page);
	priority_Q_free(&rk_res);

	/* free query */
	query_delete(qry);

close:
	indices_close(&indices);

	/* free text segment */
	text_segment_free();

	mhook_print_unfree();
	return 0;
}
