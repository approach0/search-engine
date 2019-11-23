#include "parson/parson.h"
#include "search-v3/txt2snippet.h"

#include "config.h"
#include "json-utils.h"

/* response construction buffer */
static char response[MAX_SEARCHD_RESPONSE_JSON_SZ];

/* parse JSON keyword result */
enum parse_json_kw_res {
	PARSE_JSON_KW_ABSENT_KEY,
	PARSE_JSON_KW_INVALID_TYPE,
	PARSE_JSON_KW_SUCC
};

/* parse a JSON object into query_keyword, and push into query */
static enum parse_json_kw_res
parse_json_kw_ele(JSON_Object *obj, struct query *qry)
{
	struct query_keyword kw;
	const char *type, *str;

	/* parsing keyword type */
	if (!json_object_has_value_of_type(obj, "type", JSONString))
		return PARSE_JSON_KW_ABSENT_KEY;

	type = json_object_get_string(obj, "type");

	if (0 == strcmp(type, "term"))
		kw.type = QUERY_KW_TERM;
	else if (0 == strcmp(type, "tex"))
		kw.type = QUERY_KW_TEX;
	else
		return PARSE_JSON_KW_INVALID_TYPE;

	/* for now, treat every keyword as OR operator */
	kw.op = QUERY_OP_OR;

	/* parsing keyword str value */
	if (!json_object_has_value_of_type(obj, "str", JSONString))
		return PARSE_JSON_KW_ABSENT_KEY;

	str = json_object_get_string(obj, "str");

	if (kw.type == QUERY_KW_TERM) {
		/* term */
		query_digest_txt(qry, str);
	} else {
		/* tex */
		query_push_kw(qry, str, kw.type, kw.op);
	}

	return PARSE_JSON_KW_SUCC;
}

/* parse JSON into query */
int parse_json_qry(const char* req, struct query *qry)
{
	JSON_Object *parson_obj;
	JSON_Array *parson_arr;
	int page = 0; /* page == zero indicates error */
	size_t i;

	/* parse JSON */
	JSON_Value *parson_val = json_parse_string(req);
	if (parson_val == NULL) {
		fprintf(stderr, "Parson fails to parse JSON query.\n");
		goto free;
	}

	/* get query page */
	parson_obj = json_value_get_object(parson_val);

	if (!json_object_has_value_of_type(parson_obj, "page",
	                                   JSONNumber)) {
		fprintf(stderr, "JSON query has no page number.\n");
		goto free;
	}

	page = (int)json_object_get_number(parson_obj, "page");

	/* get query keywords array (key `kw' in JSON) */
	if (!json_object_has_value_of_type(parson_obj, "kw",
	                                   JSONArray)) {
		fprintf(stderr, "JSON query does not contain kw.\n");
		page = 0;
		goto free;
	}

	parson_arr = json_object_get_array(parson_obj, "kw");

	if (parson_arr == NULL) {
		fprintf(stderr, "parson_arr returns NULL.\n");
		page = 0;
		goto free;
	}

	/* for each keyword in array ... */
	for (i = 0; i < json_array_get_count(parson_arr); i++) {
		JSON_Object *parson_arr_obj;
		enum parse_json_kw_res res;

		/* get array object[i] */
		parson_arr_obj = json_array_get_object(parson_arr, i);

		/* parse this array element */
		res = parse_json_kw_ele(parson_arr_obj, qry);

		if (PARSE_JSON_KW_SUCC != res) {
			fprintf(stderr, "keywords JSON array "
					"parse err#%d.\n", res);
			page = 0;
			goto free;
		}
	}

free:
	json_value_free(parson_val);
	return page;
}

/*
 * Response construction functions
 */
static const char
*response_head_str(enum searchd_ret_code code, int tot_pages)
{
	static char head_str[MAX_SEARCHD_RESPONSE_JSON_SZ];

	sprintf(head_str,
		"\"ret_code\": %d, "    /* return code */
		"\"ret_str\": \"%s\", " /* code-corresponding string */
		"\"tot_pages\": %u",    /* number of total pages */
		code, searchd_ret_str_map[code], tot_pages
	);

	return head_str;
}

const char *search_errcode_json(enum searchd_ret_code code)
{
	sprintf(response, "{%s}\n", response_head_str(code, 0));
	return response;
}

void json_encode_str(char *dest, const char *src, size_t max_sz)
{
	char *enc_str = json_encode_string(src);
	strncpy(dest, enc_str, max_sz);
	dest[max_sz - 1] = '\0'; /* safe guard */
	free(enc_str);
}

static const char
*response_hit_str(doc_id_t docID, float score, const char *title,
                  const char *url, const char *snippet)
{
	static char hit_str[MAX_SEARCHD_RESPONSE_JSON_SZ];
	static char enc_snippet[MAX_SNIPPET_SZ];

	/*
	 * encode into JSON string (encoded snippet is already
	 * enclosed by double quotes)
	 */
	json_encode_str(enc_snippet, snippet, MAX_SNIPPET_SZ);

	snprintf(hit_str, MAX_SEARCHD_RESPONSE_JSON_SZ,
		"{"
		"\"docid\": %u, "     /* hit docID */
		"\"score\": %.3f, "   /* hit score */
		"\"title\": %s, "     /* hit title (already encoded) */
		"\"url\": \"%s\", "   /* hit document URL */
		"\"snippet\": %s"     /* hit document snippet */
		"}",
		docID, score, title, url, enc_snippet
	);

	return hit_str;
}

static char *extract_title_string(const char *doc)
{
	char *title = NULL;
	char *sep = strstr(doc, "\n\n");

	if (NULL == sep) {
		const char no_title[] = "\"No title available.\"";
		title = malloc(strlen(no_title) + 1);
		strcpy(title, no_title);
	} else {
		unsigned int len = sep - doc;
		char *raw_title = malloc(len + 1);

		/* extract titile into raw_title */
		memcpy(raw_title, doc, len);
		raw_title[len] = '\0';

		/* JSON encode raw_title to titile */
		title = json_encode_string(raw_title);
		free(raw_title);
	}

	return title;
}

static void append_result(struct rank_result* res, void* arg)
{
	char       *url, *doc, *title; /* allocated strings */
	const char *snippet, *hit_json; /* snippet and JSON */
	size_t      url_sz, doc_sz;                /* sizes */
	list        hl_list;            /* snippet argument */

	struct rank_hit *hit = res->hit;
	doc_id_t docID = hit->docID;

	P_CAST(indices, struct indices, arg);

	/* get URL */
	url = get_blob_txt(indices->url_bi, docID, 0, &url_sz);

	/* get document text */
	doc = get_blob_txt(indices->txt_bi, docID, 1, &doc_sz);
	title = extract_title_string(doc);

	/* prepare highlighter arguments */
	hl_list = txt2snippet(doc, doc_sz, hit->occur, hit->n_occurs);

	/* get snippet */
	snippet = snippet_highlighted(&hl_list,
	                              SEARCHD_HIGHLIGHT_OPEN,
	                              SEARCHD_HIGHLIGHT_CLOSE);

	/* free highlight list */
	snippet_free_highlight_list(&hl_list);

	/* append search result */
	hit_json = response_hit_str(docID, hit->score, title, url, snippet);
	strcat(response, hit_json);

	if (res->cur + 1 < res->to)
		strcat(response, ", ");

	/* free allocated strings */
	free(url);
	free(doc);
	free(title);
}

const char *
search_results_json(ranked_results_t *rk_res, int i, struct indices *indices)
{
	struct rank_wind wind;
	int tot_pages;

	/* calculate result "window" for a specified page number */
	wind = rank_wind_calc(rk_res, i, DEFAULT_RES_PER_PAGE, &tot_pages);

	/* check requested page number legality */
	if (tot_pages == 0)
		return search_errcode_json(SEARCHD_RET_NO_HIT_FOUND);
	else if (i >= tot_pages)
		return search_errcode_json(SEARCHD_RET_ILLEGAL_PAGENUM);

	/* check window calculation validity */
	if (wind.from > 0 && wind.to > wind.from) {
		/* construct response header */
		sprintf(
			response, "{%s, \"hits\": [", response_head_str(
				SEARCHD_RET_SUCC, tot_pages
			)
		);

		/* append result JSONs */
		rank_wind_foreach(&wind, &append_result, indices);
		strcat(response, "]}\n");
	} else {
		/* not valid calculation, return error */
		return search_errcode_json(SEARCHD_RET_WIND_CALC_ERR);
	}

	return response;
}
