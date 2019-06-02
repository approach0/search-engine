#include <stdlib.h>
#include <stdio.h>
#include <evhttp.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "config.h"
#include "index.h"

#define INDEXD_DEFAULT_URI  "/index"

static const char *httpd_on_recv(const char* req, void* arg_)
{
	static char retjson[1024];

	{
		const int64_t unfree = mhook_unfree();
		printf(ES_RESET_LINE "Unfree: %ld / %u, Size: %lu / %u",
			unfree, UNFREE_CNT_INDEXER_MAINTAIN,
			index_size(), MAXIMUM_INDEX_COUNT
		);
		fflush(stdout);

		if (index_should_maintain()) {
			printf(ES_RESET_LINE "[index maintaining...]");
			fflush(stdout);
			index_maintain();

		} else if (unfree > UNFREE_CNT_INDEXER_MAINTAIN) {
			printf(ES_RESET_LINE "[index write onto disk...]");
			fflush(stdout);
			index_write();
		}
	}

	text_lexer *lex = (text_lexer*)arg_;

	indexer_index_json(req, *lex);

	sprintf(retjson, "{\"docid\": %u}", index_get_prev_docID());
	return retjson;
}

#ifdef INDEXD_PARSER_ERROR_LOG
static FILE *fh_err_output = NULL;
#endif

int parser_tex_exception_handler(char *tex, char *msg, uint64_t n_err, uint64_t total)
{
#ifdef INDEXD_PARSER_ERROR_LOG
	fprintf(fh_err_output, "%s \n", tex);
	fprintf(fh_err_output, "parsing error: %s (%lu/%lu) \n", msg, n_err, total);
#endif
	return 0;
}

int main(int argc, char* argv[])
{
	int opt;
	struct indices indices;
	doc_id_t max_doc_id;
	text_lexer lex = lex_eng_file;
	char *output_path = NULL;
	const char err_output_file[] = "./indexd-parser-error.tmp";
	unsigned short port = 8934;
	struct uri_handler uri_handlers[] = {
		{INDEXD_DEFAULT_URI, httpd_on_recv}
	};
	unsigned int len = sizeof(uri_handlers) / sizeof(struct uri_handler);

	while ((opt = getopt(argc, argv, "ho:u:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("index daemon. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | "
			       "-o <output path> (default is ./tmp) | "
			       "-u <index uri> (default is /index)"
			       "\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -o ./some/where\n", argv[0]);
			goto exit;

		case 'o':
			output_path = strdup(optarg);
			break;

		case 'u':
			if (optarg[0] != '/')
				prerr("Error: URI should start with a slash.\n", 0);
			else
				strcpy(uri_handlers[0].uri, optarg);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (output_path == NULL)
		output_path = strdup("./tmp");
	
	printf("opening output dir: %s\n", output_path);
	if (indices_open(&indices, output_path, INDICES_OPEN_RW)) {
		fprintf(stderr, "indices open failed.\n");
		goto exit;
	}
	
#ifdef INDEXD_PARSER_ERROR_LOG
	fh_err_output = fopen(err_output_file, "a");
	if (fh_err_output == NULL) {
		goto close;
	}
#endif
	
	max_doc_id = indexer_assign(&indices);
	printf("previous max docID = %u.\n", max_doc_id);
	g_lex_handler = &indexer_handle_slice;

	on_tex_index_exception = &parser_tex_exception_handler;

	printf("listening at port %u (URI: %s)...\n", port, uri_handlers[0].uri);
	httpd_run(port, uri_handlers, len, &lex);

close:
	printf("closing index...\n");
	indices_close(&indices);

#ifdef INDEXD_PARSER_ERROR_LOG
	fclose(fh_err_output);
#endif
	
exit:
	if (output_path)
		free(output_path);

	mhook_print_unfree();
	return 0;
}
