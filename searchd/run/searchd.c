#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"
#include "wstring/wstring.h"

#include "search/config.h"
#include "search/search.h"
#include "search/search-utils.h"

#include "yajl/yajl_tree.h"

#include "config.h"
#include "httpd.h"

struct searchd_args {
	struct indices *indices;
};

enum parse_json_kw_res {
	PARSE_JSON_KW_LACK_KEY,
	PARSE_JSON_KW_BAD_KEY_TYPE,
	PARSE_JSON_KW_BAD_KEY,
	PARSE_JSON_KW_BAD_VAL,
	PARSE_JSON_KW_SUCC
};

static int
parse_json_kw_ele(yajl_val obj, struct query_keyword *kw)
{
	size_t j, n_ele = obj->u.object.len;
	enum query_kw_type kw_type = QUERY_KEYWORD_INVALID;
	wchar_t kw_wstr[MAX_QUERY_WSTR_LEN] = {0};

	for (j = 0; j < n_ele; j++) {
		const char *key = obj->u.object.keys[j];
		yajl_val    val = obj->u.object.values[j];
		char *val_str = YAJL_GET_STRING(val);

		if (YAJL_IS_STRING(val)) {
			if (0 == strcmp(key, "type")) {
				if (0 == strcmp(val_str, "term"))
					kw_type = QUERY_KEYWORD_TERM;
				else if (0 == strcmp(val_str, "tex"))
					kw_type = QUERY_KEYWORD_TEX;
				else
					return PARSE_JSON_KW_BAD_KEY_TYPE;

			} else if (0 == strcmp(key, "str")) {
				wstr_copy(kw_wstr, mbstr2wstr(val_str));
			} else {
				return PARSE_JSON_KW_BAD_KEY;
			}
		} else {
			return PARSE_JSON_KW_BAD_VAL;
		}
	}

	if (wstr_len(kw_wstr) != 0 &&
	    kw_type != QUERY_KEYWORD_INVALID) {
		/* set return keyword */
		kw->type = kw_type;
		kw->pos = 0;
		wstr_copy(kw->wstr, kw_wstr);

		return PARSE_JSON_KW_SUCC;
	} else {
		return PARSE_JSON_KW_LACK_KEY;
	}
}

static uint32_t
parse_json_qry(const char* req, struct query *qry)
{
	yajl_val json_tr, json_node;
	char err_str[1024] = {0};
	const char *json_page_path[] = {"page", NULL};
	const char *json_kw_path[] = {"kw", NULL};
	uint32_t page = 0; /* page == zero indicates error */

	/* parse JSON tree */
	json_tr = yajl_tree_parse(req, err_str, sizeof(err_str));
	if (json_tr == NULL) {
		fprintf(stderr, "JSON tree parse: %s\n", err_str);
		goto exit;
	}

	/* get query page */
	json_node = yajl_tree_get(json_tr, json_page_path,
	                          yajl_t_number);
	if (json_node) {
		page = YAJL_GET_INTEGER(json_node);
	} else {
		fprintf(stderr, "no page specified in request.\n");
		goto free;
	}

	/* get query keywords array (key `kw' in JSON) */
	json_node = yajl_tree_get(json_tr, json_kw_path,
	                          yajl_t_array);

	if (json_node && YAJL_IS_ARRAY(json_node)) {
		size_t i, len = json_node->u.array.len;
		for (i = 0; i < len; i++) {
			struct query_keyword kw;
			yajl_val obj = json_node->u.array.values[i];
			int res = parse_json_kw_ele(obj, &kw);

			if (PARSE_JSON_KW_SUCC == res) {
				query_push_keyword(qry, &kw);
			} else {
				fprintf(stderr, "keywords JSON array "
				        "parse err#%d.\n", res);
				page = 0;
				goto free;
			}
		}
	}

free:
	yajl_tree_free(json_tr);

exit:
	return page;
}

char *httpd_on_recv(const char* req, void* arg)
{
	char            *ret = NULL;
	struct query     qry;
	uint32_t         page;
	//ranked_results_t srch_res; /* search results */

#ifdef DEBUG_SEARCHD
	printf("request body:\n");
	printf("%s\n", req);
#endif

	qry = query_new();
	page = parse_json_qry(req, &qry);

	if (page == 0) {
		fprintf(stderr, "requested JSON parse error.\n");
		goto reply;
	}

#ifdef DEBUG_SEARCHD
	printf("requested page: %u\n", page);

	printf("requested query: \n");
	query_print_to(qry, stdout);
	printf("\n");
#endif

reply:
	query_delete(qry);
	return ret;
}

int main(int argc, char *argv[])
{
	int                 opt;
	struct searchd_args args;
	struct indices      indices;
	unsigned short      port = SEARCHD_DEFAULT_PORT;
	char               *index_path = NULL;
	unsigned short      cache_sz = 32; /* MB */

	/* parse program arguments */
	while ((opt = getopt(argc, argv, "hi:t:p:c:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("search daemon.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -i <index path> |"
			       " -p <port> | "
			       " -c <cache size (MB)> | "
			       "\n", argv[0]);
			printf("\n");
			goto exit;

		case 'i':
			index_path = strdup(optarg);
			break;

		case 'p':
			sscanf(optarg, "%hu", &port);
			break;

		case 'c':
			sscanf(optarg, "%hu", &cache_sz);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	/*
	 * open indices
	 */
	if (index_path == NULL) {
		fprintf(stderr, "indices path not specified.\n");
		goto exit;
	}

	printf("opening index at: `%s' ...\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	/* setup cache */
	printf("setup cache size: %hu MB\n", cache_sz);
	indices_cache(&indices, cache_sz MB);

	/* open text-segment dictionary */
	printf("opening dictionary...\n");
	text_segment_init("../jieba/fork/dict");

	/* run httpd */
	args.indices = &indices;
	printf("listen on port %hu\n", port);
	httpd_run(port, &httpd_on_recv, &args);

	/* close text-segment dictionary */
	printf("closing dictionary...\n");
	text_segment_free();

close:
	/* close indices */
	printf("closing index...\n");
	indices_close(&indices);

exit:
	if (index_path)
		free(index_path);

	return 0;
}
