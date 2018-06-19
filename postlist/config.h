// #define DEBUG_MEM_POSTING

#include "term-index/config.h" /* for MAX_TERM_INDEX_ITEM_POSITIONS */

#ifdef DEBUG_MEM_POSTING
#undef N_DEBUG
#else
#define N_DEBUG
#endif

/* parameters */
#define MIN_TERM_POSTING_BUF_SZ \
	(sizeof(doc_id_t) + sizeof(uint32_t) + \
	MAX_TERM_INDEX_ITEM_POSITIONS * sizeof(position_t))

#define TERM_POSTLIST_ITEM_SZ \
	(sizeof(struct term_posting_item) + \
	 sizeof(uint32_t) * MAX_TERM_INDEX_ITEM_POSITIONS)

#define TERM_POSTLIST_SKIP_SPAN       DEFAULT_SKIPPY_SPANS
#define TERM_POSTLIST_BUF_SZ          ROUND_UP(MIN_TERM_POSTING_BUF_SZ, 4096)
#define TERM_POSTLIST_BUF_MAX_ITEMS   (TERM_POSTLIST_BUF_SZ / MIN_TERM_POSTING_BUF_SZ)  
