#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "bm25-score.h"

void BM25_init(struct BM25_scorer *bm25, float avgDocLen, float docN)
{
	const float b  = BM25_DEFAULT_B;
	bm25->docN = docN;
	bm25->avgDocLen = avgDocLen;
	bm25->frac_b_avgDocLen = b / avgDocLen;
}

float BM25_idf(struct BM25_scorer *bm25, float df)
{
	return logf((bm25->docN - df + 0.5f) / (df + 0.5f));
}

float BM25_upp(struct BM25_scorer *bm25, float idf)
{
	const float k1 = BM25_DEFAULT_K1;
	const float frac = bm25->frac_b_avgDocLen;
	return idf * (k1 + 1.f) / (1.f + k1 * frac);
}

float
BM25_partial_score(struct BM25_scorer* bm25, float tf, float idf, float doclen)
{
	const float b  = BM25_DEFAULT_B;
	const float k1 = BM25_DEFAULT_K1;
	const float frac = bm25->frac_b_avgDocLen;

	float numerator = tf * (k1 + 1.f);
	float denominator = tf + k1 * (1.f - b + frac * doclen);

	return idf * numerator / denominator;
}

void BM25_params_print(struct BM25_scorer *bm25)
{
	const float b  = BM25_DEFAULT_B;
	const float k1 = BM25_DEFAULT_K1;

	printf("docN = %.0f avgDocLen = %.2f, b = %f, k1 = %f.\n",
	       bm25->docN, bm25->avgDocLen, b, k1);
}
