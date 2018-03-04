#include <stdlib.h>
#include <stdio.h>
#include <evhttp.h>
#include <signal.h>

#include "httpd/httpd.h"
#include "config.h"
#include "index.h"

#define INDEXD_DEFAULT_URI  "/index"

static const char *httpd_on_recv(const char* req, void* arg_)
{
	printf("%s \n\n", req);

	text_lexer *lex = (text_lexer*)arg_;

	indexer_index_json(req, *lex);
	return req;
}

static FILE *fh_err_output = NULL;

int parser_tex_exception_handler(char *tex, char *msg, uint64_t n_err, uint64_t total)
{
	fprintf(fh_err_output, "%s \n", tex);
	fprintf(fh_err_output, "parsing error: %s (%lu/%lu) \n", msg, n_err, total);
	return 0;
}

int main()
{
	struct indices indices;
	doc_id_t max_doc_id;
	text_lexer lex = lex_eng_file;
	const char output_path[] = "./tmp";
	const char err_output_file[] = "./indexd-parser-error.tmp";
	unsigned short port = 8934;
	struct uri_handler uri_handlers[] = {
		{INDEXD_DEFAULT_URI, httpd_on_recv}
	};
	unsigned int len = sizeof(uri_handlers) / sizeof(struct uri_handler);
	
	if (indices_open(&indices, output_path, INDICES_OPEN_RW)) {
		fprintf(stderr, "indices open failed.\n");
		return 1;
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
	return 0;
}
