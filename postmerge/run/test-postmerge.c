#include <stdio.h>
#include <stdint.h>

#include "mhook/mhook.h"
#include "common/common.h"
#include "indices/indices.h"

#include "postmerge.h"
#include "mergecalls.h"

static int text_on_merge(uint64_t cur_min, struct postmerge *pm, void *args)
{
	for (int i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			struct term_posting_item *po_item;
			po_item = pm->cur_pos_item[i];
			printf("docID: %u, tf: %u\n", po_item->doc_id, po_item->tf);
			break;
		}
	}
	return 0;
}

static int math_on_merge(uint64_t cur_min, struct postmerge *pm, void *args)
{
	for (int i = 0; i < pm->n_postings; i++) {
		if (pm->curIDs[i] == cur_min) {
			struct math_posting_item_v2 *po_item;
			po_item = pm->cur_pos_item[i];
			printf("expID: %u, docID: %u\n", po_item->exp_id, po_item->doc_id);
			break;
		}
	}
	return 0;
}

int main()
{
	struct postmerge pm;

	char query_term[][128] = {
		"prime",
		"rickmorty",
		"absolute"
	};
	
	char query_path[][128] = {
		"./NUM/TIMES",
		"./NO/SUCH/PATH",
		"./PI/ADD"
	};

	struct indices indices;
	if(indices_open(&indices, "../indexer/tmp", INDICES_OPEN_RD)) {
		prerr("cannot open index.");
		goto close;
	}
	
	postlist_cache_set_limit(&indices.ci, 7 KB, 8 KB);

	printf("Caching begins ...\n");
	indices_cache(&indices);
	// postlist_cache_printinfo(indices.ci);

	(void)math_postlist_cache_list(indices.ci.math_cache, 1);
	(void)term_postlist_cache_list(indices.ci.term_cache, 1);

	/* Test term posting list merge */
	prinfo("Test term posting list merge");

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
				prinfo("term not indexed, an empty posting list.")
				pm_calls = NULL_POSTMERGE_CALLS;
			} else {
				prinfo("term not cached, get on-disk posting list.")
				pm_calls = mergecalls_disk_term_postlist();
				po = term_index_get_posting(indices.ti, term_id);
			}
		}
	
		postmerge_posts_add(&pm, po, pm_calls, NULL);
	}
	
	posting_merge(&pm, POSTMERGE_OP_OR, &text_on_merge, NULL);
	
	/* Test math posting list merge */
	prinfo("Test math posting list merge");

	postmerge_posts_clear(&pm);

	for (int i = 0; i < sizeof(query_path) / 128; i++) {
		char *q = query_path[i];
		struct postmerge_callbks pm_calls;
		void *po = math_postlist_cache_find(indices.ci.math_cache, q);
		printf("query path: %s\n", q);
		
		if (po) {
			prinfo("path is cached, get in-memory posting list.")
			pm_calls = mergecalls_mem_math_postlist();
		} else {
			char full_path[MAX_DIR_PATH_NAME_LEN];
			sprintf(full_path, "../indexer/tmp/prefix/%s", q);
			printf("%s\n", full_path);
			if (math_posting_exits(full_path)) {
				prinfo("path not cached, get on-disk posting list.")
				po = math_posting_new_reader(full_path);
				pm_calls = mergecalls_disk_math_postlist_v2();
			} else {
				prinfo("path not indexed, an empty posting list.")
				pm_calls = NULL_POSTMERGE_CALLS;
			}
		}
	
		postmerge_posts_add(&pm, po, pm_calls, NULL);
	}
	
	posting_merge(&pm, POSTMERGE_OP_OR, &math_on_merge, NULL);

	for (int i = 0; i < pm.n_postings; i++)
		if (math_posting_signature(pm.postings[i]))
			math_posting_free_reader(pm.postings[i]);

close:
	indices_close(&indices);
	mhook_print_unfree();
	return 0;
}
