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

struct BM25_scorer {
	float avgDocLen, docN;
	float frac_b_avgDocLen;
};

void BM25_init(struct BM25_scorer*, float avgDocLen, float docN);

float BM25_idf(struct BM25_scorer*, float df);
float BM25_upp(struct BM25_scorer*, float idf);

float BM25_partial_score(struct BM25_scorer*, float, float, float);

void BM25_params_print(struct BM25_scorer*);
