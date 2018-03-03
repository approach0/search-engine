#include <assert.h>
#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "parson/parson.h"
#include "qac.h"

#define MAX_QUERY_LOG_ITEM_SZ  (1024 * 16)

static const char *post_log_on_recv(const char* req, void* arg_)
{
	qac_index_t *qi = (qac_index_t*)arg_;
	
	JSON_Value *parson_val = json_parse_string(req);
	JSON_Object *parson_obj = json_value_get_object(parson_val);
	const char *tex = json_object_get_string(parson_obj, "tex");

	uint32_t texID = math_qac_index_uniq_tex(qi, tex);
	printf("post_log `%s' \n", tex);
	printf("TeX ID#%u ", texID);

	{
		struct qac_tex_info tex_info;
		char *tex;
		tex_info = math_qac_get(qi, texID, &tex);
		assert(NULL != tex);
		printf("%s frequency: %u \n", tex, tex_info.freq);
		free(tex);
	}

	json_value_free(parson_val);
	return req;
}

static const char *qac_query_on_recv(const char* req, void* arg_)
{
	//qac_index_t *qi = (qac_index_t*)arg_;

	return req;
}

int main()
{
	char qac_index_path[] = "./tmp";
	char err_output_file[] = "./parser-err.tmp";
	uint32_t total_uniq_tex;
	unsigned short port = 8913;
	struct uri_handler uri_handlers[] = {
		{"/post_log" , post_log_on_recv},
		{"/qac_query" , qac_query_on_recv}
	};
	unsigned int len = sizeof(uri_handlers) / sizeof(struct uri_handler);

	qac_index_t *qi = qac_index_open(qac_index_path, QAC_INDEX_WRITE_READ);
	if (qi == NULL) {
		fprintf(stderr, "QAC index open failed.\n");
		return 1;
	}

	FILE *fh_err_output = fopen(err_output_file, "a");
	if (fh_err_output == NULL) {
		fprintf(stderr, "error log open failed.\n");
		goto close;
	}

	printf("listening at port %u ...\n", port);
	httpd_run(port, uri_handlers, len, qi);

close:
	total_uniq_tex = qac_index_touch(qi, 0);
	printf("closing QAC index... (uniq_tex: %u)\n", total_uniq_tex);
	qac_index_close(qi);
	fclose(fh_err_output);

	mhook_print_unfree();
	return 0;
}
