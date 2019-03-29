struct postlist;

struct postlist *term_postlist_create_plain();

struct postlist *term_postlist_create_compressed();

#define TERM_POSTLIST_ITEM_SZ (sizeof (struct term_posting_item))
