#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include "wstring/wstring.h"
#include "txt-seg/txt-seg.h"
#include "term-index/term-index.h"
#define  LEX_PREFIX(_name) txt ## _name
#include "lex.h"

static void *term_index;

static LIST_IT_CALLBK(handle_chinese_word)
{
	LIST_OBJ(struct term_list_node, p, ln);
	char *term = wstr2mbstr(p->term);

	printf("Chinese word: `%s'\n", term);
	term_index_doc_add(term_index, term);

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(list_release, struct term_list_node,
                  ln, free(p));

int handle_chinese(char *txt)
{
	list li = LIST_NULL;

	li = text_segment(txt);
	list_foreach(&li, &handle_chinese_word, NULL);
	list_release(&li);

	return 0;
}

int handle_english(char *term)
{
	for(int i = 0; term[i]; i++)
		term[i] = tolower(term[i]);
	printf("English word:`%s'\n", term);
	term_index_doc_add(term_index, term);
	return 0;
}

void lexer_file_input(const char *path)
{
	FILE *fh = fopen(path, "r");
	if (fh) {
		txtin = fh;
		txtlex();
	} else {
		printf("file `%s' cannot be opened.\n", path);
	}
}

int main(int argc, char* argv[])
{
	int opt;
	char *path;

	/* handle program arguments */
	while ((opt = getopt(argc, argv, "hp:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("index txt document from a specified path. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <path>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./some/where/file.txt\n", argv[0]);
			printf("%s -p ./some/where\n", argv[0]);
			goto exit;
		
		case 'p':
			path = strdup(optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (path) {
		printf("index at %s\n", path);
	} else {
		printf("no path specified.\n");
		goto exit;
	}

	printf("opening dict...\n");
	text_segment_init("../jieba/clone/dict");
	printf("dict opened.\n");

	term_index = term_index_open("./tmp", TERM_INDEX_OPEN_CREATE);
	if (NULL == term_index) {
		printf("cannot create/open term index.\n");
		return 1;
	}

	term_index_doc_begin(term_index);
	lexer_file_input("./test2.txt");
	term_index_doc_end(term_index);

	printf("closing term index...\n");
	term_index_close(term_index);

	printf("closing dict...\n");
	text_segment_free();

	free(path);
exit:
	return 0;
}
