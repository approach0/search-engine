#include <stdio.h>
#include "mhook/mhook.h"
#include "term-index/term-index.h"
#include "postlist/postlist.h" /* for print_postlist() */
#include "postlist-cache.h"

int main()
{
	void *term_index = NULL;
	struct term_postlist_cache cache;

	term_index = term_index_open("../term-index/tmp/", TERM_INDEX_OPEN_EXISTS);
	if (term_index == NULL) {
		printf("index does not exists.\n");
		return 1;
	}

	cache = term_postlist_cache_new();
	
	term_postlist_cache_add(&cache, term_index);

	size_t cnt = term_postlist_cache_list(cache, 1);
	printf("cached items: %lu, total size: %lu.\n", cnt, cache.postlist_sz);

	{
		char key[] = "dog";
		struct postlist *po = term_postlist_cache_find(cache, key);
		printf("in-memory posting list [%s]:\n", key);
		printf("%p\n", po);
		if (po) print_postlist(po);
	}

	term_postlist_cache_free(cache);
	term_index_close(term_index);

	mhook_print_unfree();
	return 0;
}
