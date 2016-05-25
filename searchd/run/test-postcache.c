#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "list/list.h"
#include "tree/treap.h"
#include "term-index/term-index.h"

#include "snippet.h"
#include "mem-posting.h"

enum postcache_item_type {
	POSTCACHE_TERM_POSTING /* currently only has one type */
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

int postcache_init(struct postcache_pool *pool, uint64_t mem_limit)
{
	pool->trp_root = NULL;

	pool->trp_mem_usage = 0;
	pool->pos_mem_usage = 0;
	pool->tot_mem_limit = mem_limit;

	return 0;
}

static enum bintr_it_ret
free_postcache_item(struct bintr_ref *ref, uint32_t level, void *arg)
{
	struct postcache_item *item =
		MEMBER_2_STRUCT(ref->this_, struct postcache_item, trp_nd.bintr_nd);

	printf("free posting cache item...\n");

	bintr_detach(ref->this_, ref->ptr_to_this);
	mem_posting_release(item->posting);
	free(item);

	return BINTR_IT_CONTINUE;
}

int postcache_free(struct postcache_pool *pool)
{
	bintr_foreach((struct bintr_node **)&pool->trp_root,
	              &bintr_postorder, &free_postcache_item, NULL);

	assert(pool->trp_root == NULL);
	return 0;
}

void postcache_print_mem_usage(struct postcache_pool *pool)
{
	float trp_mem_usage_KB = (float)pool->trp_mem_usage / 1024.f;
	float pos_mem_usage_KB = (float)pool->pos_mem_usage / 1024.f;
	uint64_t total = pool->trp_mem_usage + pool->pos_mem_usage;

	printf("%.2f KB look-up structure and "
	       "%.2f KB memory posting list (total %.2f KB, %.2f%% used)",
	       trp_mem_usage_KB, pos_mem_usage_KB, (float)total / 1024.f,
	       (float)total / (float)pool->tot_mem_limit);
	printf("\n");
}

static struct mem_posting *term_posting_fork(void *term_posting)
{
	struct term_posting_item *pi;
	struct mem_posting *ret_mempost, *buf_mempost;

	const struct codec codecs[] = {
		{CODEC_PLAIN, NULL},
		{CODEC_PLAIN, NULL}
	};

	/* create memory posting to be encoded */
	ret_mempost = mem_posting_create(DEFAULT_SKIPPY_SPANS);
	mem_posting_set_enc(ret_mempost, sizeof(struct term_posting_item),
	                    codecs, sizeof(codecs));

	/* create a temporary memory posting */
	buf_mempost = mem_posting_create(MAX_SKIPPY_SPANS);

	/* start iterating term posting list */
	term_posting_start(term_posting);

	do {
		pi = term_posting_current(term_posting);
		mem_posting_write(buf_mempost, 0, pi,
		                  sizeof(struct term_posting_item));

	} while (term_posting_next(term_posting));

	/* finish iterating term posting list */
	term_posting_finish(term_posting);

	/* encode */
	mem_posting_encode(ret_mempost, buf_mempost);
	mem_posting_release(buf_mempost);
	return ret_mempost;
}

enum postcache_err
postcache_add_term_posting(struct postcache_pool *pool,
                           term_id_t term_id, void *term_posting)
{
	struct mem_posting *mem_po = term_posting_fork(term_posting);
	struct postcache_item *new = malloc(sizeof(struct postcache_item));
	struct treap_node *inserted;

	uint64_t trp_mem_usage, pos_mem_usage;

	trp_mem_usage = sizeof(struct postcache_item);
	pos_mem_usage = mem_po->n_tot_blocks * MEM_POSTING_BLOCK_SZ;

	if (pool->trp_mem_usage + trp_mem_usage +
	    pool->pos_mem_usage + pos_mem_usage > pool->tot_mem_limit)
		return POSTCACHE_EXCEED_MEM_LIMIT;

	new->posting = mem_po;
	new->type    = POSTCACHE_TERM_POSTING;
	TREAP_NODE_CONS(new->trp_nd, term_id);

	inserted = treap_insert(&pool->trp_root, &new->trp_nd);

	if (inserted == NULL) {
		mem_posting_release(mem_po);
		free(new);
		return POSTCACHE_SAME_KEY_EXISTS;
	}

	pool->trp_mem_usage += trp_mem_usage;
	pool->pos_mem_usage += pos_mem_usage;
	return POSTCACHE_NO_ERR;
}

struct postcache_item*
postcache_find(struct postcache_pool *pool, bintr_key_t key)
{
	struct bintr_ref ref;

	ref = bintr_find((struct bintr_node**)&pool->trp_root, key);

	if (ref.this_ == NULL)
		return NULL;
	else
		return MEMBER_2_STRUCT(ref.this_, struct postcache_item,
		                       trp_nd.bintr_nd);
}

int main(void)
{
	return 0;
}
