#include "common/common.h"
#include "sds/sds.h"
#include "postlist-cache.h"
#include "config.h"

struct postlist_cache postlist_cache_new()
{
	struct postlist_cache c;
	c.math_cache = math_postlist_cache_new();
	c.term_cache = term_postlist_cache_new();
	c.tot_used = 0;
	c.tot_limit = c.math_cache.limit_sz + c.term_cache.limit_sz;

	return c;
}

int postlist_cache_fork(struct postlist_cache *cache,
                        term_index_t ti, math_index_t mi)
{
	int res = 0;
	sds math_cache_path = sdsnew(mi->dir);
	math_cache_path = sdscat(math_cache_path, "/prefix");

	res |= math_postlist_cache_add(&cache->math_cache, math_cache_path);
	res |= term_postlist_cache_add(&cache->term_cache, ti);

	cache->tot_used = cache->math_cache.postlist_sz +
	                  cache->term_cache.postlist_sz;
	
	sdsfree(math_cache_path);
	return res;
}

void postlist_cache_printinfo(struct postlist_cache c)
{
	printf("Cache used: ");
	print_size(c.tot_used);
	printf(" / ");
	print_size(c.tot_limit);

	size_t cnt = math_postlist_cache_list(c.math_cache, 0);
	printf(" [%lu posting list(s)]", cnt);
	printf("\n");
}

void postlist_cache_free(struct postlist_cache cache)
{
	math_postlist_cache_free(cache.math_cache);
	term_postlist_cache_free(cache.term_cache);
}

void postlist_cache_set_limit(
	struct postlist_cache *cache,
	size_t math_cache_limit,
	size_t term_cache_limit)
{
	cache->math_cache.limit_sz = math_cache_limit;
	cache->term_cache.limit_sz = term_cache_limit;
	cache->tot_limit = math_cache_limit + term_cache_limit;
}
