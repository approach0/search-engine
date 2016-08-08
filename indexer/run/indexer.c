#include <string.h>
#include <getopt.h>
#include <stdlib.h>

#include "index.h"

struct indexer_args {
	uint64_t    indexed_files, tot_files;
	char       *path;
	text_lexer  lex;
};

void print_process(uint64_t indexed_files, uint64_t tot_files)
{
	uint64_t n_tex_correct = n_parse_tex - n_parse_err;

	printf("[process %3.0f%%] %lu/%lu file(s) indexed, "
	       "TeX parsing success rate: %.2f%%",
		   100.f * (float)indexed_files / (float)tot_files,
	       indexed_files, tot_files,
		   100.f * (float)n_tex_correct / (float)n_parse_tex);
	fflush(stdout);
}

static int foreach_file_callbk(const char *filename, void *arg)
{
	P_CAST(i_args, struct indexer_args, arg);
	char fullpath[MAX_FILE_NAME_LEN];
	FILE *fh;

	if (json_ext(filename)) {
		sprintf(fullpath, "%s/%s", i_args->path, filename);
		fh = fopen(fullpath, "r");

		//printf("opening: %s\n", fullpath);
		if (fh == NULL) {
			fprintf(stderr, "cannot open `%s', indexing aborted.\n",
			        fullpath);
			return 1;
		}

		if (indexer_index_json(fh, i_args->lex))
			fprintf(stderr, "@ %s\n", fullpath);

		i_args->indexed_files ++;
		fclose(fh);

		printf("\33[2K\r"); /* clear last line & reset cursor */
		print_process(i_args->indexed_files, i_args->tot_files);
	}

	return 0;
}

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	P_CAST(i_args, struct indexer_args, arg);
	uint64_t last = i_args->indexed_files;

	i_args->path = (char *)path;
	printf("[directory] %s\n", path);

	foreach_files_in(path, &foreach_file_callbk, i_args);

	if (i_args->indexed_files != last)
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

		print_process(1, 1);
		fclose(fh);

	} else if (dir_exists(corpus_path)) {
		struct indexer_args arg = {0, 0, NULL, lex};

		arg.tot_files = total_json_files(corpus_path);
		dir_search_podfs(corpus_path, &dir_search_callbk, &arg);

	} else {
		printf("not file/directory.\n");
	}

	printf("\ndone indexing!\n");

exit:
	indices_close(&indices);

	text_segment_free();

	if (corpus_path)
		free(corpus_path);

	return 0;
}
