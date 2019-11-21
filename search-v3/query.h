#pragma once

#include <stdint.h>
#include "linkli/list.h"

#include "config.h"

enum query_kw_type {
	QUERY_KW_INVALID,
	QUERY_KW_TEX,
	QUERY_KW_TERM
};

enum query_kw_bool_op {
	QUERY_OP_OR,
	QUERY_OP_AND,
	QUERY_OP_NOT
};

struct query_keyword {
	enum query_kw_type    type;
	enum query_kw_bool_op op;
	wchar_t               wstr[MAX_QUERY_WSTR_LEN];
	struct li_node        ln;
};

struct query {
	linkli_t keywords;
	uint32_t len;    /* in number of keywords */
	uint32_t n_math; /* number of math keywords */
	uint32_t n_term; /* number of term keywords (may contain duplicates) */
};

#define QUERY_NEW {0}
void    query_delete(struct query);

int query_push_kw(struct query*, const char*,
                  enum query_kw_type, enum query_kw_bool_op);
int query_digest_txt(struct query*, const char*);

void query_print_kw(struct query_keyword*, FILE*);
void query_print(struct query, FILE*);

struct query_keyword *query_get_kw(struct query*, int);
