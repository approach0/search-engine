#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "list/list.h"
#include "mem-index/mem-posting.h"
#include "postcache.h"

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
	P_CAST(pool, struct postcache_pool, arg);
	P_CAST(mem_po, struct mem_posting, item->posting);

	//printf("free posting cache item and memory posting list...\n");
	pool->trp_mem_usage -= sizeof(struct postcache_item);
	pool->pos_mem_usage -= mem_po->tot_sz;;

	bintr_detach(ref->this_, ref->ptr_to_this);
	mem_posting_free(mem_po);
	free(item);

	return BINTR_IT_CONTINUE;
}

int postcache_free(struct postcache_pool *pool)
{
	bintr_foreach((struct bintr_node **)&pool->trp_root,
	              &bintr_postorder, &free_postcache_item, pool);

	assert(pool->trp_root == NULL);
	return 0;
}

void postcache_print_mem_usage(struct postcache_pool *pool)
{
	float trp_mem_usage_KB = (float)pool->trp_mem_usage / 1024.f;
	float pos_mem_usage_KB = (float)pool->pos_mem_usage / 1024.f;
	uint64_t total = pool->trp_mem_usage + pool->pos_mem_usage;

	printf("%.2f KB look-up structure and "
	       "%.2f KB memory posting list "
	       "(total %.2f KB, %.2f%% of specified memory limit)",
	       trp_mem_usage_KB, pos_mem_usage_KB, (float)total / 1024.f,
	       ((float)total / (float)pool->tot_mem_limit) * 100.f);
	printf("\n");
}

struct mem_posting *postcache_fork_term_posting(void *term_posting)
{
	struct mem_posting *ret_mempost;
	struct term_posting_item *pip;

	/* create memory posting list */
	ret_mempost = mem_posting_create(DEFAULT_SKIPPY_SPANS,
	                                 mem_term_posting_with_pos_codec_calls());
	/* start iterating term posting list */
	term_posting_start(term_posting);

	do {
		size_t wr_sz = 0;

		/* get docID, TF and position array */
		pip = term_posting_cur_item_with_pos(term_posting);

		/* calculate size of term posting item with positions */
		wr_sz = sizeof(doc_id_t) + sizeof(uint32_t);
		wr_sz += pip->tf * sizeof(position_t);

		/* pass combined posting item to a single write() */
		mem_posting_write(ret_mempost, pip, wr_sz);

	} while (term_posting_next(term_posting));

	/* flush write buffer of memory posting list */
	mem_posting_write_complete(ret_mempost);

	/* finish iterating term posting list */
	term_posting_finish(term_posting);

	return ret_mempost;
}

enum postcache_err
postcache_add_term_posting(struct postcache_pool *pool,
                           term_id_t term_id, void *term_posting)
{
	struct mem_posting *mem_po = postcache_fork_term_posting(term_posting);
	struct postcache_item *new = malloc(sizeof(struct postcache_item));
	struct treap_node *inserted;

	uint64_t trp_mem_usage, pos_mem_usage;

	trp_mem_usage = sizeof(struct postcache_item);
	pos_mem_usage = mem_po->tot_sz;

	if (pool->trp_mem_usage + trp_mem_usage +
	    pool->pos_mem_usage + pos_mem_usage > pool->tot_mem_limit)
		return POSTCACHE_EXCEED_MEM_LIMIT;

	new->posting = mem_po;
	new->type    = POSTCACHE_TERM_POSTING;
	TREAP_NODE_CONS(new->trp_nd, term_id);

	inserted = treap_insert(&pool->trp_root, &new->trp_nd);

	if (inserted == NULL) {
		fprintf(stderr, "treap node with same term ID exists.\n");
		mem_posting_free(mem_po);
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

int postcache_set_mem_limit(struct postcache_pool *pool, uint64_t mem_limit)
{
	pool->tot_mem_limit = mem_limit;
	/* TODO: free memory if a lower limit is put on memory */

	return 0;
}
