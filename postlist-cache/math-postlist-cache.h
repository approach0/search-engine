#include <stdlib.h>
#include "strmap/strmap.h"

struct math_postlist_cache {
	strmap_t path_dict;
	size_t   limit_sz;
	size_t   postlist_sz;
};

struct math_postlist_cache math_postlist_cache_new(size_t);
	
void math_postlist_cache_free(struct math_postlist_cache);

int math_postlist_cache_add(struct math_postlist_cache*, const char*);
