#pragma once
#include <stdint.h>

#include "list/list.h"
#include "tree/treap.h"
#include "term-index/term-index.h"

#define POSTCACHE_POOL_LIMIT_1MB (1024 << 10)

enum postcache_item_type {
	POSTCACHE_TERM_POSTING /* currently only one type */
};

enum postcache_err {
	POSTCACHE_EXCEED_MEM_LIMIT,
	POSTCACHE_SAME_KEY_EXISTS,
	POSTCACHE_NO_ERR
};

struct postcache_item {
	void                    *posting;
	enum postcache_item_type type;
	struct treap_node        trp_nd;
};

struct postcache_pool {
	struct treap_node *trp_root;

	/* memory statics in bytes */
	uint64_t trp_mem_usage;
	uint64_t pos_mem_usage;
	uint64_t tot_mem_limit;
};

int postcache_init(struct postcache_pool*, uint64_t);

int postcache_free(struct postcache_pool*);

void postcache_print_mem_usage(struct postcache_pool*);

enum postcache_err
postcache_add_term_posting(struct postcache_pool*, term_id_t, void*);

struct postcache_item* postcache_find(struct postcache_pool*, bintr_key_t);

int postcache_set_mem_limit(struct postcache_pool*, uint64_t);

struct mem_posting *postcache_fork_term_posting(void*);
