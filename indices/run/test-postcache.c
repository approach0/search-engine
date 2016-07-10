#include <stdio.h>
#include <assert.h>

#include "mem-index/mem-posting.h"
#include "term-index/term-index.h"
#include "postcache.h"

#define TEST_TERM "significant"

int main(void)
{
	struct postcache_pool  cache_pool;
	void                  *ti;
	struct postcache_item *cache_item;

	term_id_t    term_id;
	void        *term_posting = NULL;

	printf("opening index...\n");
	ti = term_index_open("/home/tk/large-index/term",
	                             TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
		printf("term index open failed.\n");
		goto exit;
	}

	term_id = term_lookup(ti, TEST_TERM);
	if (term_id == 0) {
		printf("term id is zero.\n");
		goto exit;
	}

	printf("cache term `%s'\n", TEST_TERM);
	term_posting = term_index_get_posting(ti, term_id);

	if (term_posting == NULL) {
		printf("term posting is NULL.\n");
		goto exit;
	}

	postcache_init(&cache_pool, POSTCACHE_POOL_LIMIT_1MB);

	postcache_add_term_posting(&cache_pool, term_id, term_posting);

	postcache_print_mem_usage(&cache_pool);

	cache_item = postcache_find(&cache_pool, term_id);
	assert(cache_item != NULL);

	printf("found memory posting list:\n");
	mem_posting_print_meminfo(cache_item->posting);

	printf("free posting cache...\n");
	postcache_free(&cache_pool);

	postcache_print_mem_usage(&cache_pool);

exit:
	printf("closing index...\n");
	if (ti)
		term_index_close(ti);

	return 0;
}
