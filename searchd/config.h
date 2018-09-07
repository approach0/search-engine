#define SEARCHD_DEFAULT_PORT 8921
#define SEARCHD_DEFAULT_URI  "/search"

#define HIGHLIGHTD_DEFAULT_PORT 8961
#define HIGHLIGHTD_DEFAULT_URI  "/highlight"

//#define DEBUG_PRINT_HTTP_HEAD
//#define DEBUG_APPEND_RESULTS

#define SEARCHD_HIGHLIGHT_OPEN  "<em class=\"hl\">"
#define SEARCHD_HIGHLIGHT_CLOSE "</em>"

#define SEARCHD_DEFAULT_CACHE_MB 32 /* 32 MB */

#define SEARCHD_LOG_FILE "searchd.log"
// #define SEARCHD_LOG_ENABLE

/*
 * usually no need to log IP, because most likely
 * client is using PHP curl to relay request.
 * Thus this IP will always be localhost.
 */
//#define SEARCHD_LOG_CLIENT_IP

#define MAX_ACCEPTABLE_MATH_KEYWORDS 4
#define MAX_ACCEPTABLE_TERM_KEYWORDS 20

//#define DEBUG_JSON_ENCODE_ESCAPE
