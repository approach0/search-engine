#include "indexer/index.h" /* for text_lexer and indices */
#include "query.h"
#include "rank.h"
#include "snippet.h"

struct searcher_args {
	struct indices *indices;
	text_lexer      lex;
};

ranked_results_t
indices_run_query(struct indices*, struct query*);
