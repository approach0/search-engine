#include "proximity.h"
#include "rank.h"

/* get blob string (allocated), decode blob if needed. */
char *get_blob_string(blob_index_t, doc_id_t, bool, size_t*);

/* prepare snippet */
typedef int (*text_lexer)(FILE*);
list
prepare_snippet(struct rank_hit*, const char*, size_t);

/* math related */
#include "mnc-score.h"

void print_math_expr_at(struct indices*, doc_id_t, exp_id_t);

void print_search_results(ranked_results_t*, int, struct indices*);

char *get_expr_by_pos(char*, size_t, uint32_t);
