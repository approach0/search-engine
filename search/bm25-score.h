#include <stdint.h>
#include "postmerge/config.h" /* for MAX_MERGE_POSTINGS */
#include "config.h"

#define BM25_DEFAULT_B  0.75
#define BM25_DEFAULT_K1 1.5

struct BM25_term_i_args {
	uint32_t n_postings; /* only for printing */
	float idf[MAX_MERGE_POSTINGS];

	/* frac_b_avgDocLen is used for actual scoring for efficiency reason,
	 * avgDocLen is just for complete arguments printing. */
	float avgDocLen, frac_b_avgDocLen;

	float b, k1;
};

void  BM25_term_i_args_print(struct BM25_term_i_args*);
float BM25_term_i_score(struct BM25_term_i_args*, uint32_t, float, float);
float BM25_idf(float, float);
