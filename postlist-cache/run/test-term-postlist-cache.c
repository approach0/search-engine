#include <stdio.h>
#include "mhook/mhook.h"
#include "term-index/term-index.h"
//#include "postlist/postlist.h" /* for print_postlist() */
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

	cache = term_postlist_cache_new(3 MB);
	
	term_postlist_cache_add(&cache, term_index);
	printf("total size: %lu \n", cache.postlist_sz);

//		struct postlist *po = term_postlist_cache_find(cache, key);
//		printf("cached[%s]:\n", key);
//		print_postlist(po);

	term_postlist_cache_free(cache);
	term_index_close(term_index);

	mhook_print_unfree();
	return 0;
}
