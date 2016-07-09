#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include "indexer.h"

void lex_slice_handler(struct lex_slice *slice)
{
	indexer_handle_slice(slice);
}

static bool filename_filter(const char *filename)
{
	char *ext = filename_ext(filename);
	return (ext && strcmp(ext, ".txt") == 0);
}

struct foreach_file_args {
	uint32_t    n_files_indexed;
	const char *path;
	file_lexer  lex;
};

static int foreach_file_callbk(const char *filename, void *arg)
{
	P_CAST(fef_args, struct foreach_file_args, arg);
	char fullpath[MAX_FILE_NAME_LEN];
	uint32_t n = fef_args->n_files_indexed;

	if (1 /* filename_filter(filename) */) {
		sprintf(fullpath, "%s/%s", fef_args->path, filename);
		//printf("fullpath: %s\n", fullpath);

		{ /* print process */
			printf("\33[2K\r"); /* clear last line and reset cursor */
			printf("[files processed] %u", n);
			fflush(stdout);
		}

		index_file(fullpath, fef_args->lex);
		fef_args->n_files_indexed ++;
	}

	return 0;
}

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	struct foreach_file_args fef_args = {0, path, (file_lexer)arg};
	printf("[directory] %s\n", path);
	foreach_files_in(path, &foreach_file_callbk, &fef_args);

	if (fef_args.n_files_indexed != 0)
		printf("\n");

	return DS_RET_CONTINUE;
}

int main(int argc, char* argv[])
{
	int opt;
	file_lexer lex = lex_file_mix;
	char *corpus_path = NULL;
	const char index_path[] = "./tmp";
	const char offset_db_name[] = "offset.kvdb";
	char term_index_path[MAX_FILE_NAME_LEN];

	void        *term_index = NULL;
	math_index_t math_index = NULL;
	keyval_db_t  offset_db  = NULL;

	while ((opt = getopt(argc, argv, "hep:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("index file(s) from a specified path. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -e (english only) | -p <corpus path>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./some/where/file.txt\n", argv[0]);
			printf("%s -p ./some/where\n", argv[0]);
			goto exit;

		case 'p':
			corpus_path = strdup(optarg);
			break;

		case 'e':
			lex = lex_file_eng;
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (corpus_path) {
		printf("corpus path: %s\n", corpus_path);
	} else {
		printf("no corpus path specified.\n");
		goto exit;
	}

	/* open text segmentation dictionary */
	printf("opening dictionary...\n");
	text_segment_init("");

	/* open term index */
	printf("opening term index...\n");
	sprintf(term_index_path, "%s/term", index_path);

	mkdir_p(term_index_path);

	term_index = term_index_open(term_index_path, TERM_INDEX_OPEN_CREATE);
	if (NULL == term_index) {
		printf("cannot create/open term index.\n");
		goto exit;
	}

	/* open math index */
	math_index = math_index_open(index_path, MATH_INDEX_WRITE);
	if (NULL == math_index) {
		printf("cannot create/open math index.\n");
		goto exit;
	}

	/* open document offset key-value database */
	offset_db = keyval_db_open_under(offset_db_name, index_path,
	                                 KEYVAL_DB_OPEN_WR);
	if (offset_db == NULL) {
		printf("cannot create/open key-value DB.\n");
		goto exit;
	}

	/* set index pointers */
	index_set(term_index, math_index, offset_db);

	/* start indexing */
	if (file_exists(corpus_path)) {
		printf("[single file] %s\n", corpus_path);
		index_file(corpus_path, lex);

	} else if (dir_exists(corpus_path)) {
		dir_search_podfs(corpus_path, &dir_search_callbk, lex);

	} else {
		printf("not file/directory.\n");
	}

	printf("done indexing!\n");

exit:
	if (offset_db) {
		printf("closing key-value DB...\n");
		keyval_db_close(offset_db);
	}

	if (math_index) {
		printf("closing math index...\n");
		math_index_close(math_index);
	}

	if (term_index) {
		printf("closing term index...\n");
		term_index_close(term_index);
	}

	text_segment_free();

	if (corpus_path)
		free(corpus_path);

	return 0;
}
