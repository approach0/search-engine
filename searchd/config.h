#include "search-v3/config.h"

#define MAX_SEARCHD_RESPONSE_JSON_SZ \
	(MAX_SNIPPET_SZ * DEFAULT_N_TOP_RESULTS)

#define SEARCHD_HIGHLIGHT_OPEN  "<em class=\"hl\">"
#define SEARCHD_HIGHLIGHT_CLOSE "</em>"
