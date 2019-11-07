#include "mhook/mhook.h"
#include "common/common.h"
#include "config.h"
#include "indices-v3/indices.h"
#include "math-search.h"
#include "rank.h"
#include "print-search-results.h"

/* use MaxScore merger */
#define merger_set_iterator  ms_merger_iterator
#define merger_set_iter_next ms_merger_iter_next
#define merger_set_iter_free ms_merger_iter_free

static void print_l2_item(struct math_l2_iter_item *item)
{
	printf("[read item] doc#%u: %.2f, occurred: ", item->docID, item->score);
	for (int i = 0; i < item->n_occurs; i++) {
		printf("@%u ", item->occur[i]);
	}
	printf("\n");
}

int main()
{
	struct indices indices;
	char indices_path[] = "../indexerd/tmp";

	if(indices_open(&indices, indices_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	const char tex[] = "k(b+a)+ab+k+a";
	float threshold = 0.f;

	struct math_l2_invlist *minv;
	minv = math_l2_invlist(indices.mi, tex, &threshold);
	if (NULL == minv) {
		prerr("cannot parse tex %s", tex);
		goto close;
	}

	struct merge_set merge_set = {0};
	math_l2_invlist_iter_t l2_iter = math_l2_invlist_iterator(minv);
	merge_set.iter[0] = l2_iter;
	merge_set.upp [0] = math_l2_invlist_iter_upp(l2_iter);
	merge_set.cur [0] =  (merger_callbk_cur)math_l2_invlist_iter_cur;
	merge_set.next[0] = (merger_callbk_next)math_l2_invlist_iter_next;
	merge_set.skip[0] = (merger_callbk_skip)NULL;
	merge_set.read[0] = (merger_callbk_read)math_l2_invlist_iter_read;
	merge_set.n = 1;

	if (l2_iter == NULL) {
		prerr("cannot create level-2 iterator.");
		math_l2_invlist_free(minv);
		goto close;
	}

	printf("level-2 inverted list upperbound: %.2f\n", merge_set.upp[0]);

	ranked_results_t rk_res;
	priority_Q_init(&rk_res, DEFAULT_N_TOP_RESULTS);

	foreach (merge_iter, merger_set, &merge_set) {
		struct math_l2_iter_item item;
		merger_map_call(merge_iter, read, 0, &item, sizeof(item));
#if 1
		printf("=== Iteration ===\n");
		uint64_t cur = merger_map_call(merge_iter, cur, 0);
		printf("[cur docID] %lu\n", cur);

		print_l2_item(&item);

		math_pruner_print(l2_iter->pruner);
		ms_merger_iter_print(merge_iter, NULL);
		printf("\n");
#endif
		if (item.score > 0) {
			float score = math_score_upp(&minv->msf, item.score);

			if (!priority_Q_full(&rk_res) ||
			    score > priority_Q_min_score(&rk_res)) {

				struct rank_hit *hit = malloc(sizeof *hit);
				const int sz = sizeof(uint32_t) * item.n_occurs;
				hit->docID = item.docID;
				hit->score = score;
				hit->n_occurs = item.n_occurs;
				hit->occur = malloc(sz);
				memcpy(hit->occur, item.occur, sz);

				priority_Q_add_or_replace(&rk_res, hit);

				/* update threshold */
				threshold = priority_Q_min_score(&rk_res);
			}
		}
	}

	math_l2_invlist_iter_free(l2_iter);

	math_l2_invlist_free(minv);

	/* sort and print search results */
	priority_Q_sort(&rk_res);
	print_search_results(&indices, &rk_res, 0 /* print all pages */);
	priority_Q_free(&rk_res);

close:
	indices_close(&indices);
	mhook_print_unfree();
	return 0;
}
