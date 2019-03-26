#pragma once

#include <stdint.h>
#include "list/list.h"

#include "config.h"

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
	uint32_t           pos;
	wchar_t            wstr[MAX_QUERY_WSTR_LEN];
	struct list_node   ln;
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
void query_digest_utf8txt(struct query*, const char*);
void query_print(struct query, FILE*);
void query_delete(struct query);
void query_push_keyword_str(struct query*, const char*, enum query_kw_type);
struct query_keyword *query_keyword(struct query*, int);
