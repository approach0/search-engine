#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "search.h"
#include "query.h"

int main()
{
	struct indices indices;

	char indices_path[] = "../indexerd/tmp/";
	//char indices_path[] = "/home/tk/nvme0n1/mnt-test-opt-prune-128-compact.img";

	if(indices_open(&indices, indices_path, INDICES_OPEN_RD)) {
		prerr("indices open failed: %s", indices_path);
		goto close;
	}

	/* cache index */
	indices.ti_cache_limit = 16 MB; //DEFAULT_TERM_INDEX_CACHE_SZ;
	indices.mi_cache_limit = 2 MB; //DEFAULT_MATH_INDEX_CACHE_SZ;
	indices_cache(&indices);
	indices_print_summary(&indices);
	printf("\n");

	/* make up a query */
	struct query qry = QUERY_NEW;
	query_push_kw(&qry, "prove", QUERY_KEYWORD_TERM, QUERY_BOOL_OP_AND);
	query_push_kw(&qry, "x^2+y^2=z^2", QUERY_KEYWORD_TEX, QUERY_BOOL_OP_OR);
	query_push_kw(&qry, "x^^" /* malformed */, QUERY_KEYWORD_TEX, QUERY_BOOL_OP_OR);
	query_push_kw(&qry, "a^2+b^2=c^2", QUERY_KEYWORD_TEX, QUERY_BOOL_OP_OR);
	query_push_kw(&qry, "Pythagoras", QUERY_KEYWORD_TERM, QUERY_BOOL_OP_OR);
	query_push_kw(&qry, "theorem", QUERY_KEYWORD_TERM, QUERY_BOOL_OP_OR);
	query_push_kw(&qry, "Kunming", QUERY_KEYWORD_TERM, QUERY_BOOL_OP_NOT);
	query_push_kw(&qry, "pythagoras", QUERY_KEYWORD_TERM, QUERY_BOOL_OP_OR);

#ifdef PRINT_SEARCH_QUERIES
	query_print(qry, stdout);
	printf("\n");
#endif

	/* perform searching */
	ranked_results_t rk_res = indices_run_query(&indices, &qry);
	priority_Q_free(&rk_res);

	/* free query */
	query_delete(qry);

close:
	indices_close(&indices);
	mhook_print_unfree();
	return 0;
}
