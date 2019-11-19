#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "term-qry.h"

int
term_qry_prepare(term_index_t ti, const char* term, struct term_qry* term_qry)
{
	term_qry->term_id = term_lookup(ti, (char*)term);
	term_qry->qf = 1.f;
	term_qry->kw_str = strdup(term);

	if (0 == term_qry->term_id) {
		/* term is not found in term index vocabulary */
		term_qry->df = 0;
		return 1;
	} else {
		term_qry->df = term_index_get_df(ti, term_qry->term_id);
		return 0;
	}
}

void term_qry_release(struct term_qry *term_qry)
{
	free(term_qry->kw_str);
}

void term_qry_print(struct term_qry *term_qry)
{
	printf("`%s' (id=%u, df=%u, qf=%.0f, idf=%.2f, upp=%.2f)\n",
		term_qry->kw_str, term_qry->term_id, term_qry->df,
		term_qry->qf, term_qry->idf, term_qry->upp);
}

struct BM25_scorer prepare_bm25(term_index_t ti, struct term_qry* tqs, int n)
{
	struct BM25_scorer bm25;

	float avgDocLen = (float)term_index_get_avgDocLen(ti);
	float docN = (float)term_index_get_docN(ti);

	BM25_init(&bm25, avgDocLen, docN);

	for (int i = 0; i < n; i++) {
		float df = (float)tqs[i].df;
		tqs[i].idf = BM25_idf(&bm25, df);
		tqs[i].upp = tqs[i].qf * BM25_upp(&bm25, tqs[i].idf);
	}

	return bm25;
}

static int
array_remove(struct term_qry *tqs, int n, int j)
{
	term_qry_release(tqs + j);

	for (int i = j; i < n - 1; i++) {
		tqs[i] = tqs[i + 1];
	}

	return n - 1;
}

int term_qry_array_merge(struct term_qry* tqs, int n)
{
	for (int i = 0; i < n; i++) {
		struct term_qry* tqs_i = tqs + i;
		for (int j = i + 1; j < n; j++) {
			struct term_qry* tqs_j = tqs + j;
			if ((tqs_i->term_id > 0 && tqs_i->term_id == tqs_j->term_id) ||
			    (!tqs_i->term_id && 0 == strcmp(tqs_i->kw_str, tqs_j->kw_str))
			   ) {
			   n = array_remove(tqs, n, j);
			   tqs_i->qf += 1.f;
			   j --; /* redo at j */
			}
		}
	}

	return n;
}
