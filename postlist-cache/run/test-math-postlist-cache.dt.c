#include <stdio.h>
#include "mhook/mhook.h"
#include "postlist/postlist.h" /* for print_postlist() */
#include "math-postlist-cache.h"

int main()
{
	struct math_postlist_cache cache;
	cache = math_postlist_cache_new();
	
	math_postlist_cache_add(&cache, "../math-index/tmp/prefix");

	size_t n = math_postlist_cache_list(cache, 1);
	printf("items: %lu, total size: %lu \n", n, cache.postlist_sz);

	foreach (iter, strmap, cache.path_dict) {
		char *key = iter.cur->keystr;
		struct postlist *po = math_postlist_cache_find(cache, key);
		
		printf("in-memory posting list [%s]:\n", key);
		print_postlist(po);
		printf("\n");
	}

	math_postlist_cache_free(cache);

	mhook_print_unfree();
	return 0;
}
