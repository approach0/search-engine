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
struct rank_hit *new_hit(doc_id_t, float,
                         prox_input_t*, uint32_t);

/* get blob string (allocated), decode blob if needed. */
char *get_blob_string(blob_index_t, doc_id_t, bool, size_t*);

/* prepare snippet */
list
prepare_snippet(struct rank_hit*, const char*, size_t, text_lexer);

/* consider_top_K() */
void consider_top_K(ranked_results_t*, doc_id_t, float,
                    prox_input_t*, uint32_t);

/* set keywords values (those related to indices) */
#include "query.h"
void set_keywords_val(struct query*, struct indices*);

/* math related */
#include "mnc-score.h"

#pragma pack(push, 1)
typedef struct {
	doc_id_t    docID;
	mnc_score_t score;
	uint32_t    n_match;
	position_t  pos_arr[];
} math_score_posting_item_t;
#pragma pack(pop)

struct mem_posting_callbks math_score_posting_plain_calls();

void print_math_expr_at(struct indices*, doc_id_t, exp_id_t);
