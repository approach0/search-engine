#include "indices-v3/indices.h"
#include "search-v3/rank.h"

/* generate TREC-formated search result log */
int search_results_trec_log(struct indices*, ranked_results_t*, const char*);
