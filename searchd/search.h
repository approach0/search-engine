/*
 * query structures
 */
enum query_kw_type {
	QUERY_KEYWORD_TEX,
	QUERY_KEYWORD_TERM
};

/* for wide string converting */
#include "wstring/wstring.h"

/* for MAX_QUERY_BYTES */
#include "txt-seg/txt-seg.h"
#include "txt-seg/config.h"

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
 * searcher structures
 */
#include "list/list.h"
#include "term-index/term-index.h"
#include "keyval-db/keyval-db.h"
#include "math-index/math-index.h"

struct searcher {
	void         *ti;
	math_index_t  mi;
	keyval_db_t   keyval_db;
	bool          open_err;
};

struct searcher search_open(const char*);
void            search_close(struct searcher);

/* search method */
void search_run(struct searcher, struct query);
