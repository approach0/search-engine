#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"
#include "wstring/wstring.h"

#include "search/config.h"
#include "search/search.h"
#include "search/search-utils.h"

#include "yajl/yajl_gen.h"
#include "yajl/yajl_tree.h"

#include "config.h"
#include "utils.h"

#define MAX_SEARCHD_RESPONSE_JSON_SZ \
	(MAX_SNIPPET_SZ * DEFAULT_RES_PER_PAGE)

/* response construction buffer */
static char response[MAX_SEARCHD_RESPONSE_JSON_SZ];

/* parse JSON keyword result */
enum parse_json_kw_res {
	PARSE_JSON_KW_LACK_KEY,
	PARSE_JSON_KW_BAD_KEY_TYPE,
	PARSE_JSON_KW_BAD_KEY,
	PARSE_JSON_KW_BAD_VAL,
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
static enum parse_json_kw_res
parse_json_kw_ele(yajl_val obj, struct query_keyword *kw,
                  char **phrase)
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
				*phrase = val_str;
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
		wstr_copy(kw->wstr, kw_wstr);

		return PARSE_JSON_KW_SUCC;
	} else {
		return PARSE_JSON_KW_LACK_KEY;
	}
}

uint32_t parse_json_qry(const char* req, struct query *qry)
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
		struct query_keyword kw;
		kw.pos = 0;

		for (i = 0; i < len; i++) {
			enum parse_json_kw_res res;
			yajl_val obj = json_node->u.array.values[i];
			char *phrase = NULL;
			res = parse_json_kw_ele(obj, &kw, &phrase);

			if (PARSE_JSON_KW_SUCC == res) {
				if (kw.type == QUERY_KEYWORD_TERM && phrase)
					query_digest_utf8txt(qry, phrase);
				else
					query_push_keyword(qry, &kw);
			} else {
				fprintf(stderr, "keywords JSON array "
				        "parse err#%d.\n", res);
				page = 0;
				goto free;
			}

			kw.pos ++;
		}
	}

free:
	yajl_tree_free(json_tr);

exit:
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
	size_t len = 0;
	yajl_gen gen;
	yajl_gen_status st;

	gen = yajl_gen_alloc(NULL);

	yajl_gen_config(gen, yajl_gen_validate_utf8, 1);
	st = yajl_gen_string(gen,(const unsigned char*)src,
	                     strlen(src));

	if (yajl_gen_status_ok == st) {
		const unsigned char *buf;
		yajl_gen_get_buf(gen, &buf, &len);

		memcpy(dest, buf, len);
	}

	dest[len] = '\0';
	yajl_gen_free(gen);
}

static const char
*response_hit_str(doc_id_t docID, float score,
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
		"\"url\": \"%s\", "   /* hit document URL */
		"\"snippet\": %s" /* hit document snippet */
		"}",
		docID, score, url, enc_snippet
	);

	return hit_str;
}

static void
append_result(struct rank_hit* hit, uint32_t cnt, void* arg)
{
	char       *url, *doc;
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

	/* prepare highlighter arguments */
#ifdef DEBUG_APPEND_RESULTS
	printf("preparing highlight list...\n");
#endif
	hl_list = prepare_snippet(hit, doc, doc_sz, app_args->lex);

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
	hit_json = response_hit_str(docID, score, url, snippet);
	strcat(response, hit_json);

	if (cnt + 1 < app_args->n_results)
		strcat(response, ", ");

	/* free allocated strings */
#ifdef DEBUG_APPEND_RESULTS
	printf("free URL and doc text...\n");
#endif
	free(url);
	free(doc);
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
