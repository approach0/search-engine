#include <stdlib.h>
#include <stdio.h>

#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "tex-parser.h"

#define PARSER_ERR_LOGD_URI  "/index"

static FILE *fh_err_output = NULL;
static unsigned int total_cnt = 0;
static unsigned int error_cnt = 0;

static const char *httpd_on_recv(const char* req, void* arg_)
{
	struct tex_parse_ret ret;
	printf("%s \n\n", req);
	ret = tex_parse(req, 0, false);
	total_cnt ++;

	if (ret.code != PARSER_RETCODE_ERR) {
		subpaths_release(&ret.subpaths);
	} else {
		error_cnt ++;
		printf("%s\n", req);
		printf("%s (%u/%u)\n", ret.msg, error_cnt, total_cnt);
	}

	return req;
}

int main()
{
	const char err_output_file[] = "./parser-error-log.tmp";
	unsigned short port = 8934;
	struct uri_handler uri_handlers[] = {
		{PARSER_ERR_LOGD_URI, httpd_on_recv}
	};
	unsigned int len = sizeof(uri_handlers) / sizeof(struct uri_handler);
	
	fh_err_output = fopen(err_output_file, "a");
	if (fh_err_output == NULL)
		return 1;
	
	printf("listening at port %u ...\n", port);
	httpd_run(port, uri_handlers, len, NULL);

	fclose(fh_err_output);
	mhook_print_unfree();
	return 0;
}
