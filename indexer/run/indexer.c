#include <string.h>
#include <getopt.h>
#include <stdlib.h>

#include "index.h"

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

		if (indexer_index_json(fh, fef_args->lex))
			fprintf(stderr, "@ %s\n", fullpath);

		fef_args->n_files_indexed ++;
		fclose(fh);

		{ /* print process */
			printf("\33[2K\r"); /* clear last line and reset cursor */
			printf("[files processed] %u", fef_args->n_files_indexed);
			fflush(stdout);
		}
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
	struct indices indices;

	/* open text segmentation dictionary */
	printf("opening dictionary...\n");
	text_segment_init("");

	/* open indices for writing */
	printf("opening indices...\n");
	system("rm -rf ./tmp");
	if(indices_open(&indices, index_path, INDICES_OPEN_RW)) {
		fprintf(stderr, "indices open failed.\n");
		goto exit;
	}

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

	indexer_assign(&indices);
	g_lex_handler = &indexer_handle_slice;

	/* start indexing */
	if (file_exists(corpus_path)) {
		FILE *fh = fopen(corpus_path, "r");
		printf("[single file] %s\n", corpus_path);

		if (fh == NULL) {
			fprintf(stderr, "cannot open `%s', indexing aborted.\n",
			        corpus_path);
			goto exit;
		}

		if (indexer_index_json(fh, lex))
			fprintf(stderr, "@ %s\n", corpus_path);

		fclose(fh);

	} else if (dir_exists(corpus_path)) {
		dir_search_podfs(corpus_path, &dir_search_callbk, lex);

	} else {
		printf("not file/directory.\n");
	}

	printf("done indexing!\n");

exit:
	indices_close(&indices);

	text_segment_free();

	if (corpus_path)
		free(corpus_path);

	return 0;
}
