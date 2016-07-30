#include "postmerge.h"
#include "proximity.h"
#include "rank.h"

struct posting_merge_extra_args {
	struct indices          *indices;
	struct BM25_term_i_args *bm25args;
	ranked_results_t        *rk_res;
	prox_input_t            *prox_in;
};

/* get postmerge callback functions */
struct postmerge_callbks *get_memory_postmerge_callbks();
struct postmerge_callbks *get_disk_postmerge_callbks();

/* new rank hit */
struct rank_hit *new_hit(struct postmerge*, doc_id_t, float, uint32_t);

/* get document blob string, decode blob if needed. */
char *get_blob_string(blob_index_t, doc_id_t, bool, size_t*);

/* prepare snippet */
list prepare_snippet(struct rank_hit*, char*, size_t, text_lexer);

/* consider_top_K() */
void consider_top_K(ranked_results_t*, doc_id_t, float,
                    struct postmerge*, uint32_t);
