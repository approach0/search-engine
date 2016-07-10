#include <stdint.h>

/*
 * query structures
 */
enum query_kw_type {
	QUERY_KEYWORD_TEX,
	QUERY_KEYWORD_TERM
};

struct query_keyword {
	enum query_kw_type type;
	uint32_t           pos; /* number of keywords ahead in the same query */
	wchar_t            wstr[MAX_QUERY_WSTR_LEN];
	struct list_node   ln;
};

struct query {
	list     keywords;
	uint32_t len; /* in number of keywords */
};

/* query methods */
struct query query_new(void);
void         query_push_keyword(struct query*, const struct query_keyword*);
void         query_digest_utf8txt(struct query*, const char*);
void         query_print_to(struct query, FILE*);
void         query_delete(struct query);

/* search method */
#include "indexer/indexer.h"
#include "mem-index/mem-posting.h"
#include "postmerge.h"
#include "bm25-score.h"
#include "rank.h"
#include "math-search.h"

struct rank_set
indices_run_query(struct indices*, const struct query);
