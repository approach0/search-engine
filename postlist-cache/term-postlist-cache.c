#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"

#include "list/list.h"
#include "tree/bintr.h"

#include "term-index/term-index.h"

#include "postlist-codec/postlist-codec.h"
#include "postlist/postlist.h"
#include "postlist/term-postlist.h"

#include "config.h"
#include "term-postlist-cache.h"

struct term_postlist_cache
term_postlist_cache_new(size_t limit)
{
	struct term_postlist_cache cache;
	cache.trp_root = NULL;
	cache.limit_sz = limit;
	cache.postlist_sz = 0;

	return cache;
}

static enum bintr_it_ret
free_term_cache_treap_item(struct bintr_ref *ref, uint32_t level, void *arg)
{
	struct term_postlist_cache_item *item = MEMBER_2_STRUCT(
		ref->this_, struct term_postlist_cache_item, trp_nd.bintr_nd
	);
	PTR_CAST(mem_po, struct postlist, item->posting);

	bintr_detach(ref->this_, ref->ptr_to_this);
	postlist_free(mem_po);
	free(item);

	return BINTR_IT_CONTINUE;
}

void
term_postlist_cache_free(struct term_postlist_cache cache)
{
	bintr_foreach((struct bintr_node **)&cache.trp_root,
	              &bintr_postorder, &free_term_cache_treap_item, &cache);
	cache.postlist_sz = 0;
}

static struct postlist *fork_term_postlist(void *disk_po)
{
	struct postlist *mem_po;
	struct term_posting_item *pip;
	size_t fl_sz; /* flush size */

	/* create memory posting list */
	mem_po = term_postlist_create_compressed();
	
	/* start iterating term posting list */
	term_posting_start(disk_po);

	do {
		size_t wr_sz;

		/* get docID, TF and position array */
		pip = term_posting_cur_item_with_pos(disk_po);

		/* calculate size of term posting item with positions */
		wr_sz = sizeof(doc_id_t) + sizeof(uint32_t);
		wr_sz += pip->tf * sizeof(position_t);

#ifdef DEBUG_TERM_POSTLIST_CACHE
		printf("[%u, tf=%u], pos = [", pip->doc_id, pip->tf);
		for (uint32_t i = 0; i < pip->tf; i++)
			printf("%u ", pip->pos_arr[i]);
		printf("]\n");
#endif

		/* pass combined posting item to a single write() */
		fl_sz = postlist_write(mem_po, pip, wr_sz);
#ifdef DEBUG_TERM_POSTLIST_CACHE
		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);
#endif

	} while (term_posting_next(disk_po));

	fl_sz = postlist_write_complete(mem_po);
#ifdef DEBUG_TERM_POSTLIST_CACHE
	printf("final flush %lu bytes.\n", fl_sz);
	printf("forked posting list size: %lu\n", mem_po->tot_sz);
#else
	(void)fl_sz;
#endif

	term_posting_finish(disk_po);

	return mem_po;
}

int
term_postlist_cache_add(struct term_postlist_cache* cache, void *ti)
{
	uint32_t  termN;
	term_id_t term_id;

	void             *disk_po;
	struct postlist  *mem_po;

#ifdef ENABLE_PRINT_CACHE_TERMS
	uint32_t  df;
	char     *term;
	bool      ellp_lock = 0;
#endif

	size_t new_postlist_sz = cache->postlist_sz;

	termN = term_index_get_termN(ti);

	for (term_id = 1; term_id <= termN; term_id++) {
		disk_po = term_index_get_posting(ti, term_id);

		if (disk_po) {
#ifdef ENABLE_PRINT_CACHE_TERMS
			df = term_index_get_df(ti, term_id);
			term = term_lookup_r(ti, term_id);

			if (term_id < MAX_PRINT_CACHE_TERMS) {
				printf("`%s'(df=%u) ", term, df);

			} else if (!ellp_lock) {
				printf(" ...... ");
				ellp_lock = 1;
			}

			free(term);
			printf("\n");
#endif
			mem_po = fork_term_postlist(disk_po);

			if (cache->postlist_sz + mem_po->tot_sz > cache->limit_sz) {
				prinfo("term postlist cache reaches size limit.");
				postlist_free(mem_po);
				break;
			} else {
				cache->postlist_sz += mem_po->tot_sz;
			}

			{ /* insert the posting list into treap */
				struct treap_node *inserted;
				struct term_postlist_cache_item *new;
				new = malloc(sizeof(struct term_postlist_cache_item));
				new->posting = mem_po;
				TREAP_NODE_CONS(new->trp_nd, term_id);

				inserted = treap_insert(&cache->trp_root, &new->trp_nd);
				if (inserted == NULL) {
					prerr("treap node with same term ID exists.");
					abort();
				}
			} /* end insertion */
		}
	}
#ifdef ENABLE_PRINT_CACHE_TERMS
	printf("\n");
#endif

	return (new_postlist_sz == cache->postlist_sz);
}

///////////////////////////////////////////////////////////
//
//
//struct postcache_item*
//postcache_find(struct postcache_pool *pool, bintr_key_t key)
//{
//	struct bintr_ref ref;
//
//	ref = bintr_find((struct bintr_node**)&pool->trp_root, key);
//
//	if (ref.this_ == NULL)
//		return NULL;
//	else
//		return MEMBER_2_STRUCT(ref.this_, struct postcache_item,
//		                       trp_nd.bintr_nd);
//}
