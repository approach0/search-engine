#include <stdio.h>
#include <stdlib.h>
#include <float.h>
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

#define JSON_ARR_SEP ", "
#define JSON_RES_END "]}\n"
#define JSON_SAFE_ROOM (strlen(JSON_ARR_SEP) + strlen(JSON_RES_END) + 1)

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

		if (qry->n_math >= MAX_SEARCHD_MATH_KEYWORDS ||
		    qry->n_term >= MAX_SEARCHD_TERM_KEYWORDS)
			break;

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

	/* if buffer is large enough */
	if (strlen(response) + strlen(hit_json) + JSON_SAFE_ROOM <
		MAX_SEARCHD_RESPONSE_JSON_SZ) {
		/* append result */
		strcat(response, hit_json);
		if (res->cur + 1 < res->to)
			strcat(response, JSON_ARR_SEP);
	}

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
		strcat(response, JSON_RES_END);
	} else {
		/* not valid calculation, return error */
		return search_errcode_json(SEARCHD_RET_WIND_CALC_ERR);
	}

	return response;
}

/*
 * MPI related functions.
 */
static float hit_array_score(JSON_Array *arr, int idx)
{
	if (NULL == arr ||
	    idx >= json_array_get_count(arr))
		return -FLT_MAX;

	JSON_Object *obj = json_array_get_object(arr, idx);

	if (!json_object_has_value_of_type(obj, "score", JSONNumber)) {
		return -FLT_MAX;
	}

	return (float)json_object_get_number(obj, "score");
}

static const char *hit_object_to_string(JSON_Object *obj)
{
	static char empty[] = "{}";
	static char retstr[MAX_SEARCHD_RESPONSE_JSON_SZ];

	if (!json_object_has_value_of_type(obj, "docid",   JSONNumber) ||
	    !json_object_has_value_of_type(obj, "score",   JSONNumber) ||
	    !json_object_has_value_of_type(obj, "title",   JSONString) ||
	    !json_object_has_value_of_type(obj, "url",     JSONString) ||
	    !json_object_has_value_of_type(obj, "snippet", JSONString)) {
		return empty;
	}

#define ENC(_str) json_encode_string(_str)
	doc_id_t docID  = (doc_id_t)json_object_get_number(obj, "docid");
	float score     = (float)json_object_get_number(obj, "score");
	char *title     = ENC(json_object_get_string(obj, "title"));
	const char *url = json_object_get_string(obj, "url");
	char *snippet   = ENC(json_object_get_string(obj, "snippet"));
#undef ENC

	/* the JSON is returned by peers so it must fit in this buffer */
	sprintf(retstr, "{"
		"\"docid\": %u, "     /* hit docID */
		"\"score\": %.3f, "   /* hit score */
		"\"title\": %s, "     /* hit title */
		"\"url\": \"%s\", "   /* hit document URL */
		"\"snippet\": %s"     /* hit document snippet */
		"}",
		docID, score, title, url, snippet
	);

	free(title);
	free(snippet);
	return retstr;
}

const char *json_results_merge(char *gather_buf, int n, int page)
{
	char (*gather_res)[MAX_SEARCHD_RESPONSE_JSON_SZ];
	gather_res = (char(*)[MAX_SEARCHD_RESPONSE_JSON_SZ])gather_buf;

	/* space to store JSON values */
	JSON_Value **parson_vals = calloc(n, sizeof(JSON_Value *));

	/* merge arrays */
	JSON_Array **hit_arr = calloc(n, sizeof(JSON_Array *));
	int *cur = calloc(n, sizeof(int));

	/* calculate results window */
	int n_total_res = 0;
	int tot_pages;
	ranked_results_t rk_res; /* mock-up merged results */
	struct rank_wind wind;

	/* parse and get hit arrays */
	for (int i = 0; i < n; i++) {
		// printf("result[%d]: \n%s\n", i, gather_res[i]);
		parson_vals[i] = json_parse_string(gather_res[i]);

		if (parson_vals[i] == NULL) {
			fprintf(stderr, "Parson fails to parse JSON query.\n");
			continue;
		}

		JSON_Object *parson_obj = json_value_get_object(parson_vals[i]);

		if (!json_object_has_value_of_type(parson_obj, "hits", JSONArray)) {
			continue;
		}

		hit_arr[i] = json_object_get_array(parson_obj, "hits");
		/* count the number of total results */
		if (hit_arr[i])
			n_total_res += json_array_get_count(hit_arr[i]);
	}

	/* calculate page range */
	rk_res.n_elements = n_total_res;
	wind = rank_wind_calc(&rk_res, page - 1, DEFAULT_RES_PER_PAGE, &tot_pages);

	/* check requested page number validity */
	if (tot_pages == 0 || page > tot_pages)
		goto free;

	/* overwrite the response buffer */
	snprintf(response, MAX_SEARCHD_RESPONSE_JSON_SZ,
		"{%s, \"hits\": [", response_head_str(SEARCHD_RET_SUCC, tot_pages)
	);

	/* merge hit arrays ordered by score */
	int pos = 0;
	while (pos < wind.to) {
		/* find next max */
		float max_score = -FLT_MAX;
		for (int i = 0; i < n; i++) {
			float s = hit_array_score(hit_arr[i], cur[i]);
			if (s > max_score)
				max_score = s;
		}

		/* stop generating result on any error */
		if (max_score == -FLT_MAX)
			break;

		/* append next max result(s) */
		for (int i = 0; i < n; i++) {
			JSON_Object *obj;
			const char *json;
			float s = hit_array_score(hit_arr[i], cur[i]);
			if (s == max_score) {
				if (wind.from <= pos && pos < wind.to) {
					/* convert JSON element to string */
					obj = json_array_get_object(hit_arr[i], cur[i]);
					json = hit_object_to_string(obj);

					if (strlen(response) + strlen(json) + JSON_SAFE_ROOM <
						MAX_SEARCHD_RESPONSE_JSON_SZ) {
						/* append result string */
						strcat(response, json);

						if (pos + 1 < wind.to)
							strcat(response, JSON_ARR_SEP);
					}
				}

				cur[i] ++;
				pos ++;
			}
		}
	} /* end while */

	/* finish writing response buffer */
	strcat(response, JSON_RES_END);

free:
	/* free allocated JSON values */
	for (int i = 0; i < n; i++) {
		if (parson_vals[i])
			json_value_free(parson_vals[i]);
	}
	free(parson_vals);

	/* free merge arrays */
	free(hit_arr);
	free(cur);

	// mhook_print_unfree();

	/* response JSON message */
	if (tot_pages == 0)
		return search_errcode_json(SEARCHD_RET_NO_HIT_FOUND);
	else if (page > tot_pages)
		return search_errcode_json(SEARCHD_RET_ILLEGAL_PAGENUM);
	else
		return response;
}
