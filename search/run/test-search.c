#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "mhook/mhook.h"

#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"
#include "wstring/wstring.h"

#include "config.h"
#include "search.h"
#include "search-utils.h"

void print_res_item(struct rank_hit* hit, uint32_t cnt, void* arg_)
{
	char  *str;
	size_t str_sz;
	list   highlight_list;
	P_CAST(args, struct searcher_args, arg_);

	printf("page result#%u: doc#%u score=%.3f\n", cnt, hit->docID, hit->score);

	/* get URL */
	str = get_blob_string(args->indices->url_bi, hit->docID, 0, &str_sz);
	printf("URL: %s" "\n", str);
	free(str);

	{
		int i;
		printf("occurs: ");
		for (i = 0; i < hit->n_occurs; i++)
			printf("%u ", hit->occurs[i]);
		printf("\n");
	}
	printf("\n");

	/* get document text */
	str = get_blob_string(args->indices->txt_bi, hit->docID, 1, &str_sz);

	/* prepare highlighter arguments */
	highlight_list = prepare_snippet(hit, str, str_sz, args->lex);
	free(str);

	/* print snippet */
	snippet_hi_print(&highlight_list);
	printf("--------\n\n");

	/* free highlight list */
	snippet_free_highlight_list(&highlight_list);
}

void
print_res(ranked_results_t *rk_res, uint32_t page, struct searcher_args *args)
{
	struct rank_window win;
	uint32_t tot_pages;

	win = rank_window_calc(rk_res, page, DEFAULT_RES_PER_PAGE, &tot_pages);
	if (page >= tot_pages)
		printf("No such page. (total page(s): %u)\n", tot_pages);

	if (win.to > 0) {
		printf("page %u/%u, top result(s) from %u to %u:\n",
			   page + 1, tot_pages, win.from + 1, win.to);
		rank_window_foreach(&win, &print_res_item, args);
	}
}

int main(int argc, char *argv[])
{
	struct query_keyword keyword;
	struct query         qry;
	struct indices       indices;
	int                  opt;
	uint32_t             page = 1;
	char                *index_path = NULL;
	ranked_results_t     results;
	text_lexer           lex = lex_eng_file;
	char                *dict_path = NULL;
	struct searcher_args args;
	int                  pause = 1;

	/* a single new query */
	qry = query_new();

	while ((opt = getopt(argc, argv, "hi:p:t:m:x:d:n")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("testcase of search function.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -i <index path> |"
			       " -p <page> |"
			       " -t <term> |"
			       " -m <tex> |"
			       " -x <text> |"
			       " -d <dict> |"
			       " -n (no pause)"
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'i':
			index_path = strdup(optarg);
			break;

		case 'p':
			sscanf(optarg, "%u", &page);
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
			query_digest_utf8txt(&qry, lex_eng_file, optarg);
			break;

		case 'd':
			dict_path = strdup(optarg);
			lex = lex_mix_file;
			break;

		case 'n':
			pause = 0;
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

	/* open text segmentation dictionary */
	if (lex == lex_mix_file) {
		printf("opening dictionary...\n");
		if (text_segment_init(dict_path)) {
			fprintf(stderr, "cannot open dict.\n");
			goto exit;
		}
	}

	/*
	 * open indices
	 */
	printf("opening index at path: `%s' ...\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	/* setup cache */
	indices_cache(&indices, 32 MB);

	/*
	 * pause and continue on key press to have an idea
	 * of how long the actual search process takes.
	 */
	if (pause) {
		printf("Press Enter to Continue");
		while(getchar() != '\n');
	}

	/* search query */
	results = indices_run_query(&indices, qry);

	/* print ranked search results in pages */
	args.indices = &indices;
	args.lex = lex;
	print_res(&results, page - 1, &args);

	/* free ranked results */
	free_ranked_results(&results);

close:
	/*
	 * close indices
	 */
	printf("closing index...\n");
	indices_close(&indices);

	if (lex == lex_mix_file) {
		printf("free dictionary...\n");
		text_segment_free();
	}

exit:
	printf("existing...\n");

	/*
	 * free program arguments
	 */
	free(index_path);
	free(dict_path);

	/*
	 * free query
	 */
	query_delete(qry);

	mhook_print_unfree();
	return 0;
}
