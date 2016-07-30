#include <stdint.h>
#include "list/list.h"
#include "wstring/wstring.h"

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

/*
 * search
 */

#include "indexer/config.h" /* for MAX_CORPUS_FILE_SZ */
#include "indexer/index.h" /* for text_lexer and indices */
#include "mem-index/mem-posting.h"
#include "postmerge.h"
#include "bm25-score.h"
#include "math-search.h"
#include "proximity.h"
#include "rank.h"
#include "snippet.h"

struct posting_merge_extra_args {
	struct indices          *indices;
	struct BM25_term_i_args *bm25args;
	ranked_results_t        *rk_res;
	prox_input_t            *prox_in;
};

struct postmerge_callbks *get_memory_postmerge_callbks();
struct postmerge_callbks *get_disk_postmerge_callbks();

struct rank_hit *new_hit(struct postmerge*, doc_id_t, float, uint32_t);

struct rank_set
indices_run_query(struct indices*, const struct query);

/* search results handle */
struct highlighter_arg {
	position_t *pos_arr;
	uint32_t    pos_arr_now, pos_arr_sz;
	uint32_t    cur_lex_pos;
	list        hi_list; /* highlight list */
};

char *get_blob_string(blob_index_t, doc_id_t, bool, size_t*);

list prepare_snippet(struct rank_hit*, char*, size_t, text_lexer);
