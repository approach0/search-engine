/*
 * Searchd response (JSON) code/string
 */
enum searchd_ret_code {
	SEARCHD_RET_SUCC,
	SEARCHD_RET_EMPTY_QRY,
	SEARCHD_RET_BAD_QRY_JSON,
	SEARCHD_RET_NO_HIT_FOUND,
	SEARCHD_RET_ILLEGAL_PAGENUM,
	SEARCHD_RET_WIND_CALC_ERR,
	SEARCHD_RET_TOO_MANY_MATH_KW,
	SEARCHD_RET_TOO_MANY_TERM_KW
};

static const char searchd_ret_str_map[][128] = {
	{"Successful"},
	{"Empty or unrecognized query"},
	{"Invalid query JSON"},
	{"No hit found"},
	{"Illegal page number"},
	{"Rank window calculation error"},
	{"Too many math keywords in query"},
	{"Too many term keywords in query"}
};

/* log remote IP the request is relayed from */
void log_json_qry_ip(FILE*, const char*);

/* parse query JSON into our query structure */
uint32_t parse_json_qry(const char*, struct query*);

/* encode a C string into JSON string */
void json_encode_str(char*, const char*);

/* get response JSON with search results (for given page) */
const char *search_results_json(ranked_results_t*, int, struct indices*);

/* get response JSON with search results (all) */
const char *all_search_results_json(ranked_results_t*, struct indices*);

/* get response JSON to indicate an error */
const char *search_errcode_json(enum searchd_ret_code);

/* merge json results array into one with top K results */
const char *
json_results_merge(char*, int);

/* generate TREC-formated results log */
int search_results_trec_log(ranked_results_t*, struct indices*);
