#include <stdio.h>
#include <stdint.h>

#include "mhook/mhook.h"
#include "common/common.h"
#include "indices/indices.h"

#include "postmerge.h"
#include "mergecalls.h"

static int on_merge(uint64_t cur_min, struct postmerge *pm, void *args)
{
	printf("cur_min: %lu\n", cur_min);
	return 0;
}

int main()
{
	struct postmerge pm;

	char query_term[][128] = {
		"dog",
		"primes"
	};

	struct indices indices;
	if(indices_open(&indices, "../indexer/tmp", INDICES_OPEN_RD)) {
		prerr("cannot open index.");
		goto close;
	}

	printf("Caching begins ...\n");
	indices_cache(&indices);

	postmerge_posts_clear(&pm);

	for (int i = 0; i < sizeof(query_term) / 128; i++) {
		struct postmerge_callbks pm_calls;
		char *q = query_term[i];
		void *po = term_postlist_cache_find(indices.ci.term_cache, q);
		printf("query term: %s\n", q);
		
		if (po) {
			prinfo("term is cached, get in-memory posting list.")
			pm_calls = mergecalls_mem_term_postlist();
		} else {
			term_id_t term_id;
			term_id = term_lookup(indices.ti, q);
			if (term_id == 0) {
				prinfo("term not indexed, get empty posting list.")
				pm_calls = NULL_POSTMERGE_CALLS;
			} else {
				prinfo("term not cached, get on-disk posting list.")
				pm_calls = mergecalls_disk_term_postlist();
				po = term_index_get_posting(indices.ti, term_id);
			}
		}
	
		postmerge_posts_add(&pm, po, pm_calls, NULL);
	}
	
	posting_merge(&pm, POSTMERGE_OP_OR, &on_merge, NULL);

close:
	indices_close(&indices);
	mhook_print_unfree();
	return 0;
}
