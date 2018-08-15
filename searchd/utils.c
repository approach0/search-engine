#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"
#include "wstring/wstring.h"

#include "search/config.h"
#include "search/search.h"
#include "search/search-utils.h"

#include "parson/parson.h"

#include "config.h"
#include "utils.h"

#define MAX_SEARCHD_RESPONSE_JSON_SZ \
	(MAX_SNIPPET_SZ * DEFAULT_RES_PER_PAGE)

/* response construction buffer */
static char response[MAX_SEARCHD_RESPONSE_JSON_SZ];

/* parse JSON keyword result */
enum parse_json_kw_res {
	PARSE_JSON_KW_LACK_KEY,
	PARSE_JSON_KW_INVALID_TYPE,
	PARSE_JSON_KW_SUCC
};

/* append_result() callback function arguments */
struct append_result_args {
	struct indices *indices;
	text_lexer      lex;
	uint32_t        n_results;
};

/*
 * Query parsing related
 */
void
log_json_qry_ip(FILE *fh, const char* req)
{
	JSON_Object *parson_obj;

	JSON_Value *parson_val = json_parse_string(req);
	if (parson_val == NULL) {
		fprintf(fh, "JSON query parse error");
		goto free;
	}

	parson_obj = json_value_get_object(parson_val);
	if (!json_object_has_value_of_type(parson_obj, "ip",
	                                   JSONString)) {
		fprintf(fh, "JSON query does not contain IP");
		goto free;
	}

	fprintf(fh, "%s", json_object_get_string(parson_obj, "ip"));

free:
	json_value_free(parson_val);
}

static enum parse_json_kw_res
parse_json_kw_ele(JSON_Object *obj, size_t idx,
                  struct query *qry, text_lexer lex)
{
	struct query_keyword kw;
	const char *type, *str;

	/* set keyword postition */
	kw.pos = idx;

	/* parsing keyword type */
	if (!json_object_has_value_of_type(obj, "type", JSONString))
		return PARSE_JSON_KW_LACK_KEY;

	type = json_object_get_string(obj, "type");

	if (0 == strcmp(type, "term"))
		kw.type = QUERY_KEYWORD_TERM;
	else if (0 == strcmp(type, "tex"))
		kw.type = QUERY_KEYWORD_TEX;
	else
		return PARSE_JSON_KW_INVALID_TYPE;

	/* parsing keyword str value */
	if (!json_object_has_value_of_type(obj, "str", JSONString))
		return PARSE_JSON_KW_LACK_KEY;

	str = json_object_get_string(obj, "str");

	if (kw.type == QUERY_KEYWORD_TERM) {
		/* term */
		query_digest_utf8txt(qry, lex, str);
	} else {
		/* tex */
		wstr_copy(kw.wstr, mbstr2wstr(str));
		query_push_keyword(qry, &kw);
	}

	return PARSE_JSON_KW_SUCC;
}

uint32_t
parse_json_qry(const char* req, text_lexer lex, struct query *qry)
{
	JSON_Object *parson_obj;
	JSON_Array *parson_arr;
	uint32_t page = 0; /* page == zero indicates error */
	size_t i;

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

	page = (uint32_t)json_object_get_number(parson_obj, "page");

	/* get query keywords array (key `kw' in JSON) */
	if (!json_object_has_value_of_type(parson_obj, "kw",
	                                   JSONArray)) {
		fprintf(stderr, "JSON query does not contain hits.\n");
		page = 0;
		goto free;
	}

	parson_arr = json_object_get_array(parson_obj, "kw");

	if (parson_arr == NULL) {
		fprintf(stderr, "parson_arr returns NULL.\n");
		page = 0;
		goto free;
	}

	for (i = 0; i < json_array_get_count(parson_arr); i++) {
		JSON_Object *parson_arr_obj;
		enum parse_json_kw_res res;

		/* get array object[i] */
		parson_arr_obj = json_array_get_object(parson_arr, i);

		/* parse this array element */
		res = parse_json_kw_ele(parson_arr_obj, i, qry, lex);

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
 * Response construction related
 */
static const char
*response_head_str(enum searchd_ret_code code,
                   uint32_t tot_pages)
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

void json_encode_str(char *dest, const char *src)
{
	char *enc_str = json_encode_string(src);
	strcpy(dest, enc_str);
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
	json_encode_str(enc_snippet, snippet);

	sprintf(hit_str, "{"
		"\"docid\": %u, "     /* hit docID */
		"\"score\": %.3f, "   /* hit score */
		"\"title\": %s, "     /* hit title */
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

static void
append_result(struct rank_hit* hit, uint32_t cnt, void* arg)
{
	char       *url, *doc, *title;
	const char *snippet, *hit_json;
	size_t      url_sz, doc_sz;
	list        hl_list;
	doc_id_t    docID = hit->docID;
	float       score = hit->score;

	P_CAST(app_args, struct append_result_args, arg);
	struct indices *indices = app_args->indices;

#ifdef DEBUG_APPEND_RESULTS
	printf("appending hit (docID = %u) ...\n", docID);

	printf("occurs: ");
	{
		uint32_t i;
		for (i = 0; i < hit->n_occurs; i++)
			printf("%u ", hit->occurs[i]);
		printf("\n");
	}
#endif

	/* get URL */
#ifdef DEBUG_APPEND_RESULTS
	printf("getting URL...\n");
#endif
	url = get_blob_string(indices->url_bi, docID, 0, &url_sz);

	/* get document text */
#ifdef DEBUG_APPEND_RESULTS
	printf("getting doc text...\n");
#endif
	doc = get_blob_string(indices->txt_bi, docID, 1, &doc_sz);
	title = extract_title_string(doc);

	/* prepare highlighter arguments */
#ifdef DEBUG_APPEND_RESULTS
	printf("preparing highlight list...\n");
#endif

#ifdef DEBUG_JSON_ENCODE_ESCAPE
	{
		const char ori[] = "chars: \x003c \x0001 \x005C";
		char *esc = json_encode_string(ori);

		printf(">>> %s \n", ori);
		printf("<<< %s \n", esc);
		hit->n_occurs = 1;
		free(hit->occurs);
		hit->occurs = malloc(sizeof(position_t));
		hit->occurs[0] = 0;
		hl_list = prepare_snippet(hit, esc,
		                          doc_sz, app_args->lex);
		free(esc);
	}
#else
	hl_list = prepare_snippet(hit, doc, doc_sz, app_args->lex);
#endif

	/* get snippet */
#ifdef DEBUG_APPEND_RESULTS
	printf("getting snippet...\n");
#endif
	snippet = snippet_highlighted(&hl_list,
                                  SEARCHD_HIGHLIGHT_OPEN,
	                              SEARCHD_HIGHLIGHT_CLOSE);

	/* free highlight list */
#ifdef DEBUG_APPEND_RESULTS
	printf("free highlight list...\n");
#endif
	snippet_free_highlight_list(&hl_list);

	/* append search result */
#ifdef DEBUG_APPEND_RESULTS
	printf("append JSON result...\n");
#endif
	hit_json = response_hit_str(docID, score, title, url, snippet);
	strcat(response, hit_json);

	if (cnt + 1 < app_args->n_results)
		strcat(response, ", ");

	/* free allocated strings */
#ifdef DEBUG_APPEND_RESULTS
	printf("free URL, doc, title strings...\n");
#endif
	free(url);
	free(doc);
	free(title);
}

const char
*search_results_json(ranked_results_t *rk_res, uint32_t i,
                     struct searcher_args *se_args)
{
	struct rank_window wind;
	uint32_t tot_pages;

	/* calculate result "window" for a specified page number */
	wind = rank_window_calc(rk_res, i, DEFAULT_RES_PER_PAGE,
	                        &tot_pages);

	/* check requested page number legality */
	if ((i | tot_pages) == 0)
		return search_errcode_json(SEARCHD_RET_NO_HIT_FOUND);
	else if (i >= tot_pages)
		return search_errcode_json(SEARCHD_RET_ILLEGAL_PAGENUM);

	/* check window calculation validity */
	if (wind.to > 0) {
		/* valid window, append search results in response */
		uint32_t n_results = wind.to - wind.from;
		struct append_result_args app_args = {
			se_args->indices,
			se_args->lex,
			n_results
		};

		sprintf(
			response, "{%s, \"hits\": [", response_head_str(
				SEARCHD_RET_SUCC, tot_pages
			)
		);

		rank_window_foreach(&wind, &append_result, &app_args);
		strcat(response, "]}\n");
	} else {
		/* not valid calculation, return error */
		return search_errcode_json(SEARCHD_RET_WIND_CALC_ERR);
	}

	return response;
}

static FILE *fh_trec_output = NULL;

static void
log_trec_res(struct rank_hit* hit, uint32_t cnt, void* args)
{
	P_CAST(app_args, struct append_result_args, args);
	struct indices *indices = app_args->indices;

	size_t      url_sz;
	char *url = get_blob_string(indices->url_bi, hit->docID, 0, &url_sz);
	
	/* format: NTCIR12-MathWiki-3 1 Attenuation:3 2 58583.000000 runID */
	uint64_t qmask[MAX_MTREE] = {0};
	uint64_t dmask[MAX_MTREE] = {0};
	if (hit->n_occurs == 1){
		hit_occur_t o = hit->occurs[0];
		for (int i = 0; i < MAX_MTREE; i++) {
			qmask[i] = o.qmask[i];
			dmask[i] = o.dmask[i];
		}
	}

	fprintf(fh_trec_output, "_QRY_ID_ "
			"%u|%lx,%lx,%lx|%lx,%lx,%lx %s %u %f APPROACH0\n", hit->docID,
			qmask[0], qmask[1], qmask[2], dmask[0], dmask[1], dmask[2],
			url, ++ app_args->n_results, hit->score);
	free(url);
}

int search_results_trec_log(ranked_results_t *rk_res, struct searcher_args *args)
{
	struct rank_window wind;
	uint32_t tot_pages;
	
	fh_trec_output = fopen("trec-format-results.tmp", "w");
	if (fh_trec_output == NULL)
		return 1;

	/* calculate total pages */
	wind = rank_window_calc(rk_res, 0, DEFAULT_RES_PER_PAGE,
	                        &tot_pages);
	if (wind.to <= 0) {
		/* not valid calculation, return error */
		fclose(fh_trec_output);
		return 1;
	}

	struct append_result_args app_args = {args->indices, args->lex, 0};

	for (uint32_t i = 0; i < tot_pages; i++) {
		wind = rank_window_calc(rk_res, i, DEFAULT_RES_PER_PAGE, &tot_pages);
		rank_window_foreach(&wind, &log_trec_res, &app_args);
	}

	fclose(fh_trec_output);
	return 0;
}
