#include "codec/codec.h"
#include "invlist/invlist.h"

#include "list/list.h"
#include "tree/treap.h"

/* field index for term_posting_item */
enum {
	TI_DOCID,
	TI_TF,
	TI_N_OCCUR,
	TI_POS_ARR
};

/* reader for a term lookup entry */
struct term_invlist_entry_reader {
	invlist_iter_t inmemo_reader;
	void          *ondisk_reader;
};

/* term cache entry */
struct term_index_cache_entry {
	struct invlist    *invlist; /* in-memory inverted list */
	struct treap_node  trp_nd; /* treap key-value container */
};

/* load limited sized posting lists into cache */
int term_index_load(void *, size_t);

/* lookup a term index entry from term ID */
struct term_invlist_entry_reader
term_index_lookup(void *, term_id_t);

/* free cache structures in term index */
void term_index_cache_free(void*);

uint32_t term_index_cache_memo_usage(void*);
