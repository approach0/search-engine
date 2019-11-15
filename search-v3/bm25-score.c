#include <math.h>
#include <stdio.h>
#include "bm25-score.h"

void BM25_precalc_global(struct BM25_scorer *bm25, float avgDocLen, float docN)
{
	const float b  = BM25_DEFAULT_B;

	bm25->docN = docN;
	bm25->avgDocLen = avgDocLen;

	bm25->frac_b_avgDocLen = b / avgDocLen;
}

void BM25_precalc_idf(struct BM25_scorer *bm25, int i, float df)
{
	const float k1 = BM25_DEFAULT_K1;
	float frac = bm25->frac_b_avgDocLen;

	bm25->idf[i] = logf((bm25->docN - df + 0.5f) / (df + 0.5f));
	bm25->upp[i] = bm25->idf[i] * (k1 + 1.f) / (1.f + k1 * frac);
}

float BM25_calc_partial_score(struct BM25_scorer *bm25,
	int i, float tf, float doclen)
{
	const float b  = BM25_DEFAULT_B;
	const float k1 = BM25_DEFAULT_K1;
	float frac = bm25->frac_b_avgDocLen;

	float numerator = tf * (k1 + 1.f);
	float denominator = tf + k1 * (1.f - b + frac * doclen);

	return bm25->idf[i] * numerator / denominator;
}

void BM25_params_print(struct BM25_scorer *bm25, int n)
{
	const float b  = BM25_DEFAULT_B;
	const float k1 = BM25_DEFAULT_K1;

	printf("docN = %.0f avgDocLen = %.2f, b = %f, k1 = %f.\n",
	       bm25->docN, bm25->avgDocLen, b, k1);

	for (int i = 0; i < n; i++) {
		printf("idf[%d] = %.2f", i, bm25->idf[i]);
		printf("upperbound = %.2f\n", bm25->upp[i]);
	}
}
