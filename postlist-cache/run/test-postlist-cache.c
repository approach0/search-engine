#include <stdio.h>
#include "mhook/mhook.h"
#include "common/common.h"
#include "postlist-cache.h"

int main()
{
	struct postlist_cache cache;
	cache = postlist_cache_new();

	term_index_t  ti = NULL;
	math_index_t  mi = NULL;
	ti = term_index_open("../term-index/tmp", TERM_INDEX_OPEN_EXISTS);
	if (NULL == ti) {
		prerr("failed to open term index");
		goto free;
	}

	mi = math_index_open("../math-index/tmp", MATH_INDEX_READ_ONLY);
	if (NULL == mi) {
		prerr("failed to open math index");
		goto free;
	}

	postlist_cache_fork(&cache, ti, mi);
	postlist_cache_printinfo(cache);

	postlist_cache_free(cache);

free:
	if (ti) term_index_close(ti);
	if (mi) math_index_close(mi);

	mhook_print_unfree();
	return 0;
}
