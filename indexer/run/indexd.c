#include <stdlib.h>
#include <stdio.h>
#include <evhttp.h>
#include <signal.h>
#include <getopt.h>

#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "config.h"
#include "index.h"

#define INDEXD_DEFAULT_URI  "/index"

static const char *httpd_on_recv(const char* req, void* arg_)
{
	static char retjson[1024];
	printf("%s \n\n", req);

	text_lexer *lex = (text_lexer*)arg_;

	indexer_index_json(req, *lex);

	sprintf(retjson, "{\"docid\": %u}", index_get_prev_docID());
	return retjson;
}

static FILE *fh_err_output = NULL;

int parser_tex_exception_handler(char *tex, char *msg, uint64_t n_err, uint64_t total)
{
	fprintf(fh_err_output, "%s \n", tex);
	fprintf(fh_err_output, "parsing error: %s (%lu/%lu) \n", msg, n_err, total);
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

	while ((opt = getopt(argc, argv, "ho:p:d:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("index daemon. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | "
			       "-o <output path> (default is ./tmp)"
			       "\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -o ./some/where\n", argv[0]);
			goto exit;

		case 'o':
			output_path = strdup(optarg);
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
	
	fh_err_output = fopen(err_output_file, "a");
	if (fh_err_output == NULL) {
		goto close;
	}
	
	max_doc_id = indexer_assign(&indices);
	printf("previous max docID = %u.\n", max_doc_id);
	g_lex_handler = &indexer_handle_slice;

	on_tex_index_exception = &parser_tex_exception_handler;

	printf("listening at port %u ...\n", port);
	httpd_run(port, uri_handlers, len, &lex);

close:
	printf("closing index...\n");
	indices_close(&indices);

	fclose(fh_err_output);
	
exit:
	if (output_path)
		free(output_path);

	mhook_print_unfree();
	return 0;
}
