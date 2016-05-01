#include <stdio.h>
#include <math.h>
#include "bm25-score.h"

/*
 * This file implements an Okapi BM25 score function,
 * it is very standard and proven score function for
 * `bag of words' model. It is used as the next generation
 * of Lucene relevance score function.
 *
 * BM25 yields a non-normalized score (not necessarily
 * between zero and one. Also, BM25 does not penalize terms
 * whose tf is zero. But if the query is long, then we
 * might also need similar weighting for query terms, i.e.
 * to add frequency of term in query (query term frequency)
 * as another parameter. This is appropriate if the queries
 * are paragraph long, but unnecessary for short queries.
 *
 * See links below for more:
 * https://en.wikipedia.org/wiki/Okapi_BM25
 * http://nlp.stanford.edu/IR-book/html/-
 * htmledition/okapi-bm25-a-non-binary-model-1.html
 */

void BM25_term_i_args_print(struct BM25_term_i_args *args)
{
	uint32_t i;
	printf("avgDocLen = %f, b = %f, k1 = %f, b/avgDocLen = %f.\n",
	       args->avgDocLen, args->b, args->k1, args->frac_b_avgDocLen);

	if (args->n_postings > 0) {
		for (i = 0; i < args->n_postings; i++) {
			printf("idf[%d] = %f", i, args->idf[i]);
			if (i + 1 != args->n_postings)
				printf(", ");
		}

		printf(".\n");
	}
}

float BM25_term_i_score(struct BM25_term_i_args* args, uint32_t i, float tf, float doclen)
{
	float idf = args->idf[i], frac = args->frac_b_avgDocLen,
	      b = args->b, k1 = args->k1;

	float numerator = tf * (k1 + 1.f);
	float denominator = tf + k1 * (1.f - b + frac * doclen);

	return idf * (numerator / denominator);
}

float BM25_idf(float df, float docN)
{
	return logf((docN - df + 0.5f) / (df + 0.5f));
}
