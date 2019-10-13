#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "httpd/httpd.h"
#include "parson/parson.h"
#include "txt-seg/lex.h"

#include "config.h"
#include "indices-v3/indices.h"

static int get_json_val(const char *json, const char *key, char *val)
{
	JSON_Value *parson_val = json_parse_string(json);
	JSON_Object *parson_obj;

	if (parson_val == NULL) {
		json_value_free(parson_val);
		return 1;
	}

	parson_obj = json_value_get_object(parson_val);
	strcpy(val, json_object_get_string(parson_obj, key));

	json_value_free(parson_val);
	return 0;
}

static int
parser_exception(struct indexer *indexer, const char *tex, char *msg)
{
	fprintf(stderr, "%s\n", tex);
	prerr("%s\n", msg);
	return 0;
}

static int consider_index_maintain(struct indexer *indexer)
{
	struct indices *indices = indexer->indices;

	if (indexer_should_maintain(indexer)) {
		printf("\n[index maintaining ...]\n");
		fflush(stdout);

		indexer_maintain(indexer);
		sleep(30);

	} else if ((indices->n_doc + 1) % MAINTAIN_UNFREE_CNT == 0) {
		printf("\n[flushing index ...]\n");
		fflush(stdout);

		indexer_spill(indexer);
		sleep(2);
	}
	return 0;
}

static const char *httpd_on_index(const char* req, void* arg_)
{
	static char retjson[1024];
	uint32_t n_doc = 0;
	P_CAST(indexer, struct indexer, arg_);

	char *url_field = indexer->url_field;
	char *txt_field = indexer->txt_field;

	if (get_json_val(req, "url", url_field)) {
		fprintf(stderr, "JSON: get URL field failed.\n");
		goto ret;
	}

	if (get_json_val(req, "text", txt_field)) {
		fprintf(stderr, "JSON: get TXT field failed.\n");
		goto ret;
	}

	if (strlen(url_field) == 0 || strlen(txt_field) == 0) {
		fprintf(stderr, "JSON: URL/TXT field strlen is zero.\n");
		goto ret;
	}

	printf(ES_RESET_LINE); /* clear line */
	consider_index_maintain(indexer);

	n_doc = indexer_write_all_fields(indexer);

	/* print maintainer related stats */
	const int64_t unfree = mhook_unfree();
	printf("Unfree: %ld, Doc: %u per %u, Index segments: %lu / %u, ",
		unfree, indexer->indices->n_doc, MAINTAIN_UNFREE_CNT,
		term_index_size(indexer->indices->ti), MAXIMUM_INDEX_COUNT
	);
	printf("%lu TeX parsed, error rate: %.2f %%.", indexer->n_parse_tex,
		100.f * indexer->n_parse_err / indexer->n_parse_tex);
	fflush(stdout);

ret:
	sprintf(retjson, "{\"docid\": %u}", n_doc);
	return retjson;
}

int main()
{
	struct uri_handler handlers[] = {
		{INDEXD_DEFAULT_URI, &httpd_on_index}
	};
	struct indices indices;
	struct indexer *indexer;
	unsigned short port = INDEXD_DEFAULT_PORT;

	indices_open(&indices, "./tmp", INDICES_OPEN_RW);

	indexer = indexer_alloc(&indices, &lex_eng_file, &parser_exception);

	printf("listening at %u:%s ...\n", port, handlers[0].uri);
	httpd_run(port, handlers,
		sizeof handlers / sizeof(struct uri_handler), indexer);

	printf("closing index...\n");
	indexer_free(indexer);
	indices_close(&indices);

	mhook_print_unfree();
	return 0;
}
