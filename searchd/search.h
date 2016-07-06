#include <stdint.h>

/*
 * query structures
 */
enum query_kw_type {
	QUERY_KEYWORD_TEX,
	QUERY_KEYWORD_TERM
};

/* for wide string converting */
#include "wstring/wstring.h"

/* for MAX_TXT_SEG_BYTES & MAX_TXT_SEG_LEN */
#include "txt-seg/txt-seg.h"
#define MAX_TERM_BYTES MAX_TXT_SEG_BYTES

/* consider both math & term */
#define MAX_QUERY_BYTES     (MAX_TXT_SEG_BYTES * 32)
#define MAX_QUERY_WSTR_LEN  (MAX_TXT_SEG_LEN * 32)

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

/*
 * indices structures
 */
#include "list/list.h"
#include "term-index/term-index.h"
#include "keyval-db/keyval-db.h"
#include "math-index/math-index.h"
#include "mem-index/postcache.h"

struct indices {
	void                 *ti;
	math_index_t          mi;
	keyval_db_t           keyval_db;
	bool                  open_err;
	struct postcache_pool postcache;
};

struct indices indices_open(const char*);
void           indices_close(struct indices*);

#define MB * POSTCACHE_POOL_LIMIT_1MB

void indices_cache(struct indices*, uint64_t);

/* search method */
#include "mem-index/mem-posting.h"
#include "postmerge.h"
#include "bm25-score.h"
#include "rank.h"
#include "math-search.h"

struct rank_set
indices_run_query(struct indices*, const struct query);
