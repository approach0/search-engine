#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>

#include "mhook/mhook.h"
#include "config.h"
#include "index.h"

static volatile int force_stop = 0;

struct indexer_args {
	uint64_t    indexed_files, tot_files;
	char       *path;
	text_lexer  lex;
};

static void signal_handler(int sig) {
	switch (sig) {
		case SIGTERM:
		case SIGHUP:
		case SIGQUIT:
		case SIGINT:
			force_stop = 1;
			break;
	}
}

void print_process(uint64_t indexed_files, uint64_t tot_files)
{
	uint64_t n_tex_correct = n_parse_tex - n_parse_err;

	printf("[ %3.0f%% ] %lu/%lu file(s), unfree: %ld, "
	       "TeX parsing success: %lu/%lu",
	       100.f * (float)indexed_files / (float)tot_files,
	       indexed_files, tot_files, mhook_unfree(),
		   n_tex_correct, n_parse_tex);
	fflush(stdout);
}

static int foreach_file_callbk(const char *filename, void *arg)
{
	P_CAST(i_args, struct indexer_args, arg);
	char fullpath[MAX_FILE_NAME_LEN];
	FILE *fh;

	if (force_stop) return 1;

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

	if (mhook_unfree() > UNFREE_CNT_INDEXER_MAINTAIN)
		index_maintain();

	if (force_stop) {
		printf("\n");
		printf("indexing aborted.\n");

		return DS_RET_STOP_ALLDIR;
	}

	if (i_args->indexed_files != last)
		printf("\n");
	else
		printf("no file in this directory.\n");

	return DS_RET_CONTINUE;
}

int main(int argc, char* argv[])
{
	int opt, overwrite = 0;
	text_lexer lex = lex_eng_file;
	char *corpus_path = NULL;
	char *dict_path = NULL;
	const char index_path[] = DEFAULT_INDEXER_OUTPUT_DIR;
	struct indices indices;
	doc_id_t max_doc_id;

	while ((opt = getopt(argc, argv, "hop:d:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("index file(s) from a specified path. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | "
			       "-d <dict path> | "
			       "-p <corpus path> | "
			       "-o (overwrite)"
			       "\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./some/where/file.txt\n", argv[0]);
			printf("%s -p ./some/where\n", argv[0]);
			goto exit;

		case 'p':
			corpus_path = strdup(optarg);
			break;

		case 'd':
			dict_path = strdup(optarg);
			lex = lex_mix_file;
			break;

		case 'o':
			overwrite = 1;
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * check program arguments.
	 */
	if (corpus_path) {
		printf("corpus path: %s\n", corpus_path);
	} else {
		printf("no corpus path specified.\n");
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

	/* remove output directory if we are overwriting index */
	if (overwrite) {
		printf("remove old index...\n");
		system("rm -rf " DEFAULT_INDEXER_OUTPUT_DIR);
	}

	/* open indices for writing */
	printf("opening indices...\n");
	if(indices_open(&indices, index_path, INDICES_OPEN_RW)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	/* set signal handler */
	signal(SIGINT, signal_handler);

	/* initialize indexer */
	max_doc_id = indexer_assign(&indices);
	printf("previous max docID = %u.\n", max_doc_id);
	g_lex_handler = &indexer_handle_slice;

	/* start indexing */
	if (file_exists(corpus_path)) {
		FILE *fh = fopen(corpus_path, "r");
		printf("[single file] %s\n", corpus_path);

		if (fh == NULL) {
			fprintf(stderr, "cannot open `%s', indexing aborted.\n",
			        corpus_path);
			goto close;
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

close:
	/*
	 * close indices
	 */
	printf("closing index...\n");
	indices_close(&indices);

	if (lex == lex_mix_file) {
		printf("closing dict...\n");
		text_segment_free();
	}

exit:
	printf("existing...\n");

	/*
	 * free program arguments
	 */
	if (corpus_path)
		free(corpus_path);

	if (dict_path)
		free(dict_path);

	mhook_print_unfree();
	return 0;
}
