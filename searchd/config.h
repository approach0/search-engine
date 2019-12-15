#include "search-v3/config.h"

#define MAX_SEARCHD_RESPONSE_JSON_SZ \
	(MAX_SNIPPET_SZ * DEFAULT_K_TOP_RESULTS)

#define SEARCHD_HIGHLIGHT_OPEN  "<em class=\"hl\">"
#define SEARCHD_HIGHLIGHT_CLOSE "</em>"

#define MAX_SEARCHD_MATH_KEYWORDS 2
#define MAX_SEARCHD_TERM_KEYWORDS 10

#define SEARCHD_DEFAULT_PORT 8921
#define SEARCHD_DEFAULT_URI  "/search"

#define CLUSTER_MASTER_NODE 0
#define CLUSTER_MAX_QRY_BUF_SZ 65530

#define SEARCHD_LOG_FILE "searchd.log"
#define OUTPUT_TREC_FILE "trec-format-results.tmp"

//#define DEBUG_LOG_STATS_SYNC
