#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>

#undef NDEBUG
#include <assert.h>

#include "mem-index/mem-posting.h"

#include "config.h"
#include "postmerge.h"
#include "bm25-score.h"
#include "proximity.h"
#include "math-search.h"
#include "search.h"
#include "search-utils.h"

struct add_postinglist_arg {
	struct indices          *indices;
	struct postmerge        *pm;
	uint32_t                 docN;
	uint32_t                 posting_idx;
	struct BM25_term_i_args *bm25args;
};

struct add_postinglist {
	void *posting;
	struct postmerge_callbks *postmerge_callbks;
};

static struct add_postinglist
term_postinglist(char *kw_utf8, struct add_postinglist_arg *apfm_args)
{
	struct add_postinglist ret;
	void                  *ti;
	term_id_t              term_id;
	uint32_t               df;
	float                 *idf;
	float                  docN;

	/* get variables for short-hand */
	ti = apfm_args->indices->ti;
	term_id = term_lookup(ti, kw_utf8);
	docN = (float)apfm_args->docN;
	idf = apfm_args->bm25args->idf;

	printf("posting list[%u] of keyword `%s'(termID: %u)...\n",
	       apfm_args->posting_idx, kw_utf8, term_id);

	if (term_id == 0) {
		/* if term is not found in our dictionary */
		ret.posting = NULL;
		ret.postmerge_callbks = NULL;

		/* get IDF number */
		df = 0;
		idf[apfm_args->posting_idx] = BM25_idf((float)df, docN);

		printf("keyword not found.\n");
	} else {
		/* otherwise, get on-disk or cached posting list */
		struct postcache_item *cache_item =
			postcache_find(&apfm_args->indices->postcache, term_id);

		if (NULL != cache_item) {
			/* if this term is already cached */
			ret.posting = cache_item->posting;
			ret.postmerge_callbks = get_memory_postmerge_callbks();

			printf("using cached posting list.\n");
		} else {
			/* otherwise read posting from disk */
			ret.posting = term_index_get_posting(ti, term_id);
			ret.postmerge_callbks = get_disk_postmerge_callbks();
			printf("using on-disk posting list.\n");
		}

		/* get IDF number */
		df = term_index_get_df(ti, term_id);
		idf[apfm_args->posting_idx] = BM25_idf((float)df, docN);
	}

	return ret;
}
