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
	printf("\n");
	return req;
}

void print_suggestion(struct rank_hit* hit, uint32_t cnt, void* arg_)
{
	qac_index_t *qi = (qac_index_t*)arg_;
	struct qac_tex_info tex_info;
	char *tex;

	tex_info = math_qac_get(qi, hit->docID, &tex);
	assert(NULL != tex);

	printf("tex#%u, popularity: %u, score: %.2f \n",
	       hit->docID, tex_info.freq, hit->score);
	printf("%s \n\n", tex);
	free(tex);
}

static char res_json_str[4096 * 8];
void write_suggestion(struct rank_hit* hit, uint32_t now, uint32_t end, void* arg_)
{
	qac_index_t *qi = (qac_index_t*)arg_;
	char tmp_str[4096 * 8];
	char *json_tex;
	struct qac_tex_info tex_info;
	char *tex;

	tex_info = math_qac_get(qi, hit->docID, &tex);
	assert(NULL != tex);

	json_tex = json_encode_string(tex);
	free(tex);

	sprintf(tmp_str, "{"
		"\"tex\": %s, "
		"\"score\": %.3f,"
		"\"freq\": %u"
		"}", json_tex, hit->score, tex_info.freq
	);
	free(json_tex);

	strcat(res_json_str, tmp_str);
	if (now != end - 1)
		strcat(res_json_str, ", ");
}

static const char *qac_query_on_recv(const char* req, void* arg_)
{
	qac_index_t *qi = (qac_index_t*)arg_;

	JSON_Value *parson_val = json_parse_string(req);
	JSON_Object *parson_obj = json_value_get_object(parson_val);
	const char *tex = json_object_get_string(parson_obj, "tex");
	struct rank_window win;
	uint32_t tot_pages;

	printf("QAC query: %s \n\n", tex);
	if (tex == NULL) {
		res_json_str[0] = '\0';
		strcat(res_json_str, "{\"qac\": [");
		strcat(res_json_str, "]}");
		return res_json_str;
	}
	ranked_results_t rk_res = math_qac_query(qi, tex);

	win = rank_window_calc(&rk_res, 0, DEFAULT_QAC_SUGGESTIONS, &tot_pages);
 	rank_window_foreach(&win, &print_suggestion, qi);

	{ /* return json list here */
		res_json_str[0] = '\0';
		strcat(res_json_str, "{\"qac\": [");
		rank_window_foreach2(&win, &write_suggestion, qi);
		strcat(res_json_str, "]}");
	}

	json_value_free(parson_val);
	priority_Q_free(&rk_res);

	printf("%s\n", res_json_str);
	return res_json_str;
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
