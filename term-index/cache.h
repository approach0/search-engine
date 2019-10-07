#include "codec/codec.h"
#include "invlist/invlist.h"

#include "list/list.h"
#include "tree/treap.h"

struct term_invlist_entry_reader {
	invlist_iter_t inmemo_reader;
	void          *ondisk_reader;
	uint32_t       df;
};

struct term_index_cache_entry {
	struct invlist    *invlist; /* in-memory inverted list */
	struct treap_node  trp_nd;
	uint32_t           df; /* document frequency */
};

int term_index_load(void *, size_t);

struct term_invlist_entry_reader
term_index_lookup(void *, term_id_t);

void term_index_cache_free(void*);
