#include <stdint.h>
#include "term-index/term-index.h"
#include "bm25-score.h"

struct term_qry {
	char *kw_str;
	uint32_t term_id;
	uint32_t df;

	/* bm25 scoring */
	float qf; /* query term frequency */
	float idf;
	float upp;
};

int  term_qry_prepare(term_index_t, const char*, struct term_qry*);
void term_qry_release(struct term_qry*);
void term_qry_print(struct term_qry*);

/* merge duplicates query terms */
int term_qry_array_merge(struct term_qry*, int);

/* prepare BM25 parameters, should be called after term_qry_array_merge(). */
struct BM25_scorer
prepare_bm25(term_index_t, uint32_t, uint32_t, struct term_qry*, int);
