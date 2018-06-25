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

#include "indexer/index.h" /* for text_lexer and indices */
#include "proximity.h"
#include "rank.h"
#include "snippet.h"
#include "search-utils.h"

struct math_expr_search_arg {
	ranked_results_t *rk_res;
	struct indices   *indices;

	doc_id_t          cur_docID;
	uint32_t          n_occurs;
	uint32_t          max_score;
	position_t        pos_arr[MAX_HIGHLIGHT_OCCURS];
	prox_input_t      prox_in[MAX_MERGE_POSTINGS];
};

static int
on_merge(uint64_t cur_min, struct postmerge* pm, void* args)
{
	struct math_expr_score_res res = {0};
	PTR_CAST(mesa, struct math_extra_score_arg, args);
	PTR_CAST(esa, struct math_expr_search_arg, mesa->expr_srch_arg);

	if (cur_min == 0)
		goto add_hit;
	
	/* score calculation */
	res = math_expr_prefix_score_on_merge(cur_min, pm, mesa, esa->indices);

	if (esa->cur_docID != 0 && esa->cur_docID != res.doc_id) {
add_hit:
		prox_set_input(esa->prox_in + 0, esa->pos_arr, esa->n_occurs);
		consider_top_K(esa->rk_res, esa->cur_docID, esa->max_score,
		               esa->prox_in, 1);
//		printf("Final doc#%u score: %u, ", esa->cur_docID, esa->max_score);
//		printf("pos: ");
//		for (uint32_t i = 0; i < esa->n_occurs; i++) {
//			printf("%u ", esa->pos_arr[i]);
//		}
//		printf("\n");

		esa->n_occurs  = 0;
		esa->max_score = 0;
	}

//	printf("doc#%u, exp#%u, score: %u\n",
//	       res.doc_id, res.exp_id, res.score);

	if (esa->n_occurs < MAX_HIGHLIGHT_OCCURS)
		esa->pos_arr[esa->n_occurs ++] = res.exp_id;
	esa->max_score = (esa->max_score > res.score) ? esa->max_score : res.score;

	esa->cur_docID = res.doc_id;
	return 0;
}

#define LEXER_FUN lex_eng_file
void print_res_item(struct rank_hit* hit, uint32_t cnt, void *arg)
{
	char  *str;
	size_t str_sz;
	list   highlight_list;
	PTR_CAST(indices, struct indices, arg);
	printf("page result#%u: doc#%u score=%.3f\n", cnt, hit->docID, hit->score);

	/* get URL */
	str = get_blob_string(indices->url_bi, hit->docID, 0, &str_sz);
	printf("URL: %s" "\n", str);
	free(str);

//	{
//		int i;
//		printf("occurs: ");
//		for (i = 0; i < hit->n_occurs; i++)
//			printf("%u ", hit->occurs[i]);
//		printf("\n");
//	}
	printf("\n");

	/* get document text */
	str = get_blob_string(indices->txt_bi, hit->docID, 1, &str_sz);

	/* prepare highlighter arguments */
	highlight_list = prepare_snippet(hit, str, str_sz, LEXER_FUN);
	free(str);

	/* print snippet */
	snippet_hi_print(&highlight_list);
	printf("--------\n\n");

	/* free highlight list */
	snippet_free_highlight_list(&highlight_list);
}

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
void
print_res(ranked_results_t *rk_res, uint32_t page, struct indices *indices)
{
	struct rank_window wind;
	uint32_t i, from_page = page, to_page = page, tot_pages = 1;
	wind = rank_window_calc(rk_res, 0, DEFAULT_RES_PER_PAGE, &tot_pages);

	if (page == 0) {
		from_page = 1;
		to_page = tot_pages;
	}

	for (i = from_page - 1; i < MIN(to_page, tot_pages); i++) {
		printf("page %u/%u\n", i + 1, tot_pages);
		wind = rank_window_calc(rk_res, i, DEFAULT_RES_PER_PAGE, &tot_pages);
		rank_window_foreach(&wind, &print_res_item, indices);
	}
}

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

	struct math_expr_search_arg args;
	args.rk_res    = &rk_res;
	args.indices   = &indices;
	args.cur_docID = 0;
	args.n_occurs  = 0;
	args.max_score = 0;

	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);
	math_expr_search(&indices, query, srch_policy, &on_merge, &args);
	priority_Q_sort(&rk_res);

	print_res(&rk_res, 1, &indices); /* print page 1 only */

	free_ranked_results(&rk_res);

close:
	indices_close(&indices);

exit:
	free(index_path);
	mhook_print_unfree();
	return 0;
}
