#include "term-index/config.h" /* for MAX_TERM_INDEX_ITEM_POSITIONS */

//#define DEBUG_POSTLIST

#ifdef DEBUG_POSTLIST
#undef NDEBUG
#else
#define NDEBUG
#endif

/* parameters */
#define MIN_TERM_POSTING_BUF_SZ \
	(sizeof(doc_id_t) + sizeof(uint32_t) + \
	MAX_TERM_INDEX_ITEM_POSITIONS * sizeof(position_t))

#define TERM_POSTLIST_ITEM_SZ \
	(sizeof(struct term_posting_item) + \
	 sizeof(uint32_t) * MAX_TERM_INDEX_ITEM_POSITIONS)

#ifdef DEBUG_POSTLIST
#define TERM_POSTLIST_SKIP_SPAN       2
#else
#define TERM_POSTLIST_SKIP_SPAN       DEFAULT_SKIPPY_SPANS
#endif

#ifdef DEBUG_POSTLIST
#define TERM_POSTLIST_BUF_SZ          (MIN_TERM_POSTING_BUF_SZ * 2)
#else
#define TERM_POSTLIST_BUF_SZ          ROUND_UP(MIN_TERM_POSTING_BUF_SZ, 4096)
#endif

#define TERM_POSTLIST_BUF_MAX_ITEMS   (TERM_POSTLIST_BUF_SZ / MIN_TERM_POSTING_BUF_SZ)  
