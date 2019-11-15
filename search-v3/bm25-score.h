#pragma once
/*
 * Okapi BM25 scoring
 * BM25 yields a non-normalized score (not necessarily
 * between zero and one) and it does not penalize terms
 * whose tf is zero.
 *
 * The TF scoring function is:
 *                         tf * (k1 + 1)
 * TF(q, d) = ------------------------------------------
 *             tf + k1 * (1 - b + b(doclen / avgDocLen))
 *
 * where default k1 = 1.5 (in [1.2, 2.0]), and b = 0.75.
 *
 * The TF upperbound is (k1 + 1) / (1 + k1 (b / avgDocLen)).
 *
 *                docN - df + 0.5
 * IDF(q) = log( ----------------- )
 *                   df + 0.5
 */

#include "config.h"

struct BM25_scorer {
	float avgDocLen, docN;
	float idf[MAX_SEARCH_INVLISTS];

	/* fast lookup values */
	float frac_b_avgDocLen;
	float upp[MAX_SEARCH_INVLISTS];
};

void  BM25_precalc_global(struct BM25_scorer*, float avgDocLen, float docN);
void  BM25_precalc_idf(struct BM25_scorer*, int i, float df);

float
BM25_calc_partial_score(struct BM25_scorer*, int i, float tf, float doclen);

void  BM25_params_print(struct BM25_scorer*, int n);
