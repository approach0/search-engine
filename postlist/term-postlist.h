struct postlist;

struct postlist *term_postlist_create_plain();

struct postlist *term_postlist_create_compressed();

#include "term-index/config.h" /* for MAX_TERM_INDEX_ITEM_POSITIONS */
#define TERM_POSTLIST_ITEM_SZ \
	(sizeof(struct term_posting_item) + \
	MAX_TERM_INDEX_ITEM_POSITIONS * sizeof(position_t))
