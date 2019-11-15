#include <stdint.h>
#include "term-index/term-index.h"
#include "bm25-score.h"

struct term_qry {
	char *kw_str;
	uint32_t term_id;
	uint32_t df;
	uint32_t qf; /* query term frequency */

	/* bm25 scoring */
	float idf;
	float upp;
};

int  term_qry_prepare(term_index_t, const char*, struct term_qry*);
void term_qry_release(struct term_qry*);
void term_qry_print(struct term_qry*);

struct BM25_scorer prepare_bm25(term_index_t, struct term_qry*, int);

/* merge duplicates query terms */
int term_qry_array_merge(struct term_qry*, int);
