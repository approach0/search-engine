#include <assert.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "search.h"

int main(int argc, char *argv[])
{
	struct query_keyword keyword;
	struct query    qry;
	struct searcher se;
	int             opt;
	char           *index_path = NULL;

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
	 * open searcher
	 */
	printf("opening index...\n");
	se = search_open(index_path);
	if (se.open_err) {
		printf("index open failed.\n");
		goto close;
	}

close:
	/*
	 * close searcher
	 */
	printf("closing index...\n");
	search_close(se);

	/*
	 * free program arguments
	 */
exit:
	if (index_path)
		free(index_path);

	/*
	 * free other program modules
	 */
	query_delete(qry);
	text_segment_free();
	return 0;
}
