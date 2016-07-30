/*
 * query
 */

#include "query.h"

/*
 * search
 */

#include "indexer/index.h" /* for text_lexer and indices */
#include "rank.h"
#include "snippet.h"

ranked_results_t
indices_run_query(struct indices*, const struct query);
