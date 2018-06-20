#include <stdio.h>
#include "mhook/mhook.h"
#include "postlist/postlist.h" /* for print_postlist() */
#include "math-postlist-cache.h"

int main()
{
	struct math_postlist_cache cache;
	cache = math_postlist_cache_new(500);
	
	math_postlist_cache_add(&cache, "./tmp/prefix");
	printf("total size: %lu \n", cache.postlist_sz);

	foreach (iter, strmap, cache.path_dict) {
		char *key = iter.cur->keystr;
		printf("cached[%s]:\n", key);
		print_postlist(cache.path_dict[[key]]);
	}

	math_postlist_cache_free(cache);

	mhook_print_unfree();
	return 0;
}
