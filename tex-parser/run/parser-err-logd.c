#include <stdlib.h>
#include <stdio.h>

#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "parson/parson.h"
#include "tex-parser.h"

#define PARSER_ERR_LOGD_URI  "/parser"

static FILE *fh_err_output = NULL;
static unsigned int total_cnt = 0;
static unsigned int error_cnt = 0;

static const char *httpd_on_recv(const char* req, void* arg_)
{
	struct tex_parse_ret ret;
	printf("%s \n\n", req);

	JSON_Value *parson_val = json_parse_string(req);
	JSON_Object *parson_obj = json_value_get_object(parson_val);
	const char *tex = json_object_get_string(parson_obj, "tex");

	ret = tex_parse(tex, 0, false, false);
	total_cnt ++;

	if (ret.code != PARSER_RETCODE_ERR) {
		subpaths_release(&ret.subpaths);
	} else {
		error_cnt ++;
		fprintf(fh_err_output, "%s\n", tex);
		fprintf(fh_err_output, "%s (%u/%u)\n", ret.msg, error_cnt, total_cnt);
	}

	json_value_free(parson_val);
	return req;
}

int main()
{
	const char err_output_file[] = "./parser-err-log.tmp";
	unsigned short port = 8998;
	struct uri_handler uri_handlers[] = {
		{PARSER_ERR_LOGD_URI, httpd_on_recv}
	};
	unsigned int len = sizeof(uri_handlers) / sizeof(struct uri_handler);
	
	fh_err_output = fopen(err_output_file, "w");
	if (fh_err_output == NULL)
		return 1;
	
	printf("listening at port %u ...\n", port);
	httpd_run(port, uri_handlers, len, NULL);

	fclose(fh_err_output);
	mhook_print_unfree();
	return 0;
}
