#pragma once
#include <stdlib.h>
#include <stdint.h>

#include "list/list.h"
#include "tree/treap.h"

struct term_postlist_cache_item {
	void                    *posting;
	struct treap_node        trp_nd;
};

struct term_postlist_cache {
	struct treap_node *trp_root;
	void              *term_index;
	size_t limit_sz;
	size_t postlist_sz;
};

struct term_postlist_cache term_postlist_cache_new(void);
	
void term_postlist_cache_free(struct term_postlist_cache);

int term_postlist_cache_add(struct term_postlist_cache*, void*);

void*
term_postlist_cache_find(struct term_postlist_cache, char*);

size_t
term_postlist_cache_list(struct term_postlist_cache, int);
