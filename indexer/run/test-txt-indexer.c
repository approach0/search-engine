#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>

#include "wstring/wstring.h"
#include "txt-seg/txt-seg.h"
#include "term-index/term-index.h"

#define  LEX_PREFIX(_name) txt ## _name
#include "lex.h"

#include "filesys.h"
#include "config.h"

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

int foreach_file_callbk(const char *filename, void *arg)
{
	char *path = (char*) arg;
	char *ext = filename_ext(filename);
	char fullpath[MAX_FILE_NAME_LEN];

	if (ext && strcmp(ext, ".txt") == 0) {
		sprintf(fullpath, "%s/%s", path, filename);
		printf("[txt file] %s\n", fullpath);

		term_index_doc_begin(term_index);
		lexer_file_input(fullpath);
		term_index_doc_end(term_index);
	}

	return 0;
}

enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	printf("[directory] %s\n", path);
	foreach_files_in(path, &foreach_file_callbk, (void*)path);
	return DS_RET_CONTINUE;
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
		printf("index path %s\n", path);
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

	if (file_exists(path)) {
		printf("[file] %s\n", path);
		term_index_doc_begin(term_index);
		lexer_file_input(path);
		term_index_doc_end(term_index);
	} else if (dir_exists(path)) {
		dir_search_podfs(path, &dir_search_callbk, NULL);
	} else {
		printf("not file/directory.\n");
	}

	printf("closing term index...\n");
	term_index_close(term_index);

	printf("closing dict...\n");
	text_segment_free();

	free(path);
exit:
	return 0;
}
