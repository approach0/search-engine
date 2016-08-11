/* parse query JSON into our query structure */
uint32_t parse_json_qry(const char*, struct query*);

/* encode a C string into JSON string */
void json_encode_str(char*, const char*);

/* get response JSON with search results */
const char *searchd_response(ranked_results_t*, uint32_t, struct indices*);
