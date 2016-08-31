#pragma once

#include <stdint.h>
#include "list/list.h"

/*
 * query structures
 */
enum query_kw_type {
	QUERY_KEYWORD_TEX,
	QUERY_KEYWORD_TERM,
	QUERY_KEYWORD_INVALID
};

struct query_keyword {
	enum query_kw_type type;
	uint32_t           pos; /* number of keywords ahead in the same query */
	wchar_t            wstr[MAX_QUERY_WSTR_LEN];

	struct list_node   ln;

	/* values to be assinged */
	int64_t            post_id;
	uint64_t           df;
};

struct query {
	list     keywords;
	uint32_t len; /* in number of keywords */
	uint32_t n_math; /* number of math keywords */
	uint32_t n_term; /* number of term keywords */
};

/* query methods */
struct query query_new(void);
void query_push_keyword(struct query*, const struct query_keyword*);
void query_digest_utf8txt(struct query*, text_lexer, const char*);
void query_print_to(struct query, FILE*);
void query_delete(struct query);
void query_sort_by_df(const struct query*);
void query_uniq_by_post_id(struct query*);
