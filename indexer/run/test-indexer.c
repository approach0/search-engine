#include <string.h>
#include <getopt.h>
#include <stdlib.h>

#include "indexer.h"

void lex_slice_handler(struct lex_slice *slice)
{
	indexer_handle_slice(slice);
}

static bool json_ext(const char *filename)
{
	char *ext = filename_ext(filename);
	return (ext && strcmp(ext, ".json") == 0);
}

struct foreach_file_args {
	uint32_t    n_files_indexed;
	const char *path;
	text_lexer  lex;
};

static int foreach_file_callbk(const char *filename, void *arg)
{
	P_CAST(fef_args, struct foreach_file_args, arg);
	char fullpath[MAX_FILE_NAME_LEN];
	uint32_t n = fef_args->n_files_indexed;
	FILE *fh;

	if (json_ext(filename)) {
		sprintf(fullpath, "%s/%s", fef_args->path, filename);
		fh = fopen(fullpath, "r");

		//printf("opening: %s\n", fullpath);
		if (fh == NULL) {
			fprintf(stderr, "cannot open `%s', indexing aborted.\n",
			        fullpath);
			return 1;
		}

		{ /* print process */
			printf("\33[2K\r"); /* clear last line and reset cursor */
			printf("[files processed] %u", n);
			fflush(stdout);
		}

		index_json_file(fh, fef_args->lex);
		fef_args->n_files_indexed ++;
		fclose(fh);
	}

	return 0;
}

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	struct foreach_file_args fef_args = {0, path, (text_lexer)arg};
	printf("[directory] %s\n", path);
	foreach_files_in(path, &foreach_file_callbk, &fef_args);

	if (fef_args.n_files_indexed != 0)
		printf("\n");

	return DS_RET_CONTINUE;
}

int main(int argc, char* argv[])
{
	int opt;
	text_lexer lex = lex_mix_file;
	char *corpus_path = NULL;
	const char index_path[] = "./tmp";
	const char offset_db_name[] = "offset.kvdb";
	const char blob_index_url_name[] = "url";
	const char blob_index_txt_name[] = "doc";
	char       path[MAX_FILE_NAME_LEN];

	void        *term_index = NULL;
	math_index_t math_index = NULL;
	keyval_db_t  offset_db  = NULL;
	blob_index_t blob_index_url = NULL;
	blob_index_t blob_index_txt = NULL;

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
			lex = lex_eng_file;
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
	sprintf(path, "%s/term", index_path);

	mkdir_p(path);

	term_index = term_index_open(path, TERM_INDEX_OPEN_CREATE);
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

	/* open blob index */
	sprintf(path, "%s/%s", index_path, blob_index_url_name);
	blob_index_url = blob_index_open(path, "w+");
	if (NULL == blob_index_url) {
		printf("cannot create/open URL blob index.\n");
		goto exit;
	}

	sprintf(path, "%s/%s", index_path, blob_index_txt_name);
	blob_index_txt = blob_index_open(path, "w+");
	if (NULL == blob_index_txt) {
		printf("cannot create/open text blob index.\n");
		goto exit;
	}

	/* set index pointers */
	index_set(term_index, math_index, offset_db,
	          blob_index_url, blob_index_txt);

	/* start indexing */
	if (file_exists(corpus_path)) {
		FILE *fh = fopen(corpus_path, "r");
		printf("[single file] %s\n", corpus_path);

		if (fh == NULL) {
			fprintf(stderr, "cannot open `%s', indexing aborted.\n",
			        corpus_path);
			goto exit;
		}

		index_json_file(fh, lex);
		fclose(fh);

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

	if (blob_index_url) {
		printf("closing URL blob index...\n");
		blob_index_close(blob_index_url);
	}

	if (blob_index_txt) {
		printf("closing text blob index...\n");
		blob_index_close(blob_index_txt);
	}

	text_segment_free();

	if (corpus_path)
		free(corpus_path);

	return 0;
}
