#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>

#include "wstring/wstring.h"
#include "txt-seg/txt-seg.h"
#include "term-index/term-index.h"
#include "keyval-db/keyval-db.h"

#define  LEX_PREFIX(_name) txt ## _name
#include "lex.h"

#include "filesys.h"
#include "config.h"

static void *term_index;
keyval_db_t  keyval_db;

static LIST_IT_CALLBK(handle_chinese_word)
{
	LIST_OBJ(struct term_list_node, p, ln);
	char *term = wstr2mbstr(p->term);

	//printf("Chinese word: `%s'\n", term);
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

	//printf("English word:`%s'\n", term);
	term_index_doc_add(term_index, term);
	return 0;
}

static void lexer_file_input(const char *path)
{
	FILE *fh = fopen(path, "r");
	if (fh) {
		txtin = fh;
		txtlex();
		fclose(fh);
	} else {
		printf("cannot open `%s'.\n", path);
	}
}

static void index_txt_document(const char *fullpath)
{
	size_t val_sz = strlen(fullpath) + 1;
	doc_id_t new_docID;

	term_index_doc_begin(term_index);
	lexer_file_input(fullpath);
	new_docID = term_index_doc_end(term_index);

	if(keyval_db_put(keyval_db, &new_docID, sizeof(doc_id_t),
	                 (void *)fullpath, val_sz)) {
		printf("put error: %s\n", keyval_db_last_err(keyval_db));
		return;
	}
}

static int foreach_file_callbk(const char *filename, void *arg)
{
	char *path = (char*) arg;
	//char *ext = filename_ext(filename);
	char fullpath[MAX_FILE_NAME_LEN];

	//if (ext && strcmp(ext, ".txt") == 0) {
		sprintf(fullpath, "%s/%s", path, filename);
		//printf("[txt file] %s\n", fullpath);

		index_txt_document(fullpath);
		if (term_index_maintain(term_index))
			printf("\r[term index merging...]");
	//}

	return 0;
}

static enum ds_ret
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
	char *keyval_db_path /* key value DB tmp string*/;
	char *path /* corpus path */;
	const char index_path[] = "./tmp";

	while ((opt = getopt(argc, argv, "hp:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("index txt document from a specified path. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <corpus path>\n", argv[0]);
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
		printf("corpus path %s\n", path);
	} else {
		printf("no corpus path specified.\n");
		goto exit;
	}

	printf("opening dict...\n");
	text_segment_init("../jieba/fork/dict");
	printf("dict opened.\n");

	printf("opening term index...\n");
	term_index = term_index_open(index_path, TERM_INDEX_OPEN_CREATE);
	if (NULL == term_index) {
		printf("cannot create/open term index.\n");
		return 1;
	}

	keyval_db_path = (char *)malloc(1024);
	strcpy(keyval_db_path, index_path);
	strcat(keyval_db_path, "/kv_doc_path.bin");
	printf("opening key-value DB (%s)...\n", keyval_db_path);
	keyval_db = keyval_db_open(keyval_db_path, KEYVAL_DB_OPEN_WR);
	free(keyval_db_path);
	if (keyval_db == NULL) {
		printf("cannot create/open key-value DB.\n");
		goto keyval_db_fails;
	}

	if (file_exists(path)) {
		printf("[single file] %s\n", path);
		index_txt_document(path);
	} else if (dir_exists(path)) {
		dir_search_podfs(path, &dir_search_callbk, NULL);
	} else {
		printf("not file/directory.\n");
	}

	printf("closing key-value DB...\n");
	keyval_db_close(keyval_db);

keyval_db_fails:
	printf("closing term index...\n");
	term_index_close(term_index);

	printf("closing dict...\n");
	text_segment_free();

	free(path);
exit:
	return 0;
}
