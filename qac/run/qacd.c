#include <assert.h>
#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "qac.h"

static const char *post_log_on_recv(const char* req, void* arg_)
{
	qac_index_t *qi = (qac_index_t*)arg_;

	uint32_t texID = math_qac_index_uniq_tex(qi, req);
	printf("post_log `%s' \n", req);
	printf("TeX ID#%u ", texID);

	{
		struct qac_tex_info tex_info;
		char *tex;
		tex_info = math_qac_get(qi, texID, &tex);
		assert(NULL != tex);
		printf("%s frequency: %u \n", tex, tex_info.freq);
		free(tex);
	}

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
	printf("closing QAC index...\n");
	qac_index_close(qi);
	fclose(fh_err_output);

	mhook_print_unfree();
	return 0;
}
