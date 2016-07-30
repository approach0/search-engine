#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"
#include "wstring/wstring.h"

#include "config.h"
#include "search.h"
#include "search-utils.h"

void print_res_item(struct rank_hit* hit, uint32_t cnt, void* arg)
{
	char  *str;
	size_t str_sz;
	list   highlight_list;
	P_CAST(indices, struct indices, arg);

	printf("result#%u: doc#%u score=%.3f\n", cnt, hit->docID, hit->score);

	{
		int i;
		printf("occurs: ");
		for (i = 0; i < hit->n_occurs; i++)
			printf("%u ", hit->occurs[i]);
		printf("\n");
	}

	/* get URL */
	str = get_blob_string(indices->url_bi, hit->docID, 0, &str_sz);
	printf("URL: %s" "\n\n", str);

	/* get document text */
	str = get_blob_string(indices->txt_bi, hit->docID, 1, &str_sz);

	/* prepare highlighter arguments */
	highlight_list = prepare_snippet(hit, str, str_sz, lex_eng_file);

	/* print snippet */
	snippet_hi_print(&highlight_list);
	printf("--------\n\n");

	/* free highlight list */
	snippet_free_highlight_list(&highlight_list);
}

uint32_t
print_all_rank_res(ranked_results_t *rk_res, struct indices *indices)
{
	struct rank_window win;
	uint32_t tot_pages, page = 0;

	do {
		win = rank_window_calc(rk_res, page, DEFAULT_RES_PER_PAGE, &tot_pages);
		if (win.to > 0) {
			printf("page#%u (from %u to %u):\n",
			       page + 1, win.from, win.to);
			rank_window_foreach(&win, &print_res_item, indices);
			page ++;
		}
	} while (page < tot_pages);

	return tot_pages;
}

int main(int argc, char *argv[])
{
	struct query_keyword keyword;
	struct query         qry;
	struct indices       indices;
	int                  opt;
	char                *index_path = NULL;
	ranked_results_t     results;
	uint32_t             res_pages;

	/* initialize text segmentation module */
	text_segment_init("../jieba/fork/dict");

	/* a single new query */
	qry = query_new();

	while ((opt = getopt(argc, argv, "hp:t:m:x:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("testcase of search function.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -p <index path> |"
			       " -t <term> |"
			       " -m <tex> |"
			       " -x <text>"
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			keyword.type = QUERY_KEYWORD_TERM;
			wstr_copy(keyword.wstr, mbstr2wstr(optarg));
			query_push_keyword(&qry, &keyword);
			break;

		case 'm':
			keyword.type = QUERY_KEYWORD_TEX;
			keyword.pos = 0; /* do not care for now */
			wstr_copy(keyword.wstr, mbstr2wstr(optarg));
			query_push_keyword(&qry, &keyword);
			break;

		case 'x':
			query_digest_utf8txt(&qry, optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * check program arguments.
	 */
	if (index_path == NULL || qry.len == 0) {
		printf("not enough arguments.\n");
		goto exit;
	}

	/*
	 * print program arguments.
	 */
	printf("index path: %s\n", index_path);
	query_print_to(qry, stdout);

	/*
	 * open indices
	 */
	printf("opening index...\n");
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	/* setup cache */
	indices_cache(&indices, 32 MB);

	/* search query */
	results = indices_run_query(&indices, qry);

	/* print ranked search results in pages */
	res_pages = print_all_rank_res(&results, &indices);
	printf("result(s): %u pages.\n", res_pages);

	/* free ranked results */
	free_ranked_results(&results);

close:
	/*
	 * close indices
	 */
	printf("closing index...\n");
	indices_close(&indices);

exit:
	/*
	 * free program arguments
	 */
	if (index_path)
		free(index_path);

	/*
	 * free other program modules
	 */
	query_delete(qry);
	text_segment_free();
	return 0;
}
