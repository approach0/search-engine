#include "mhook/mhook.h"
#include "common/common.h"
#include "config.h"
#include "indices-v3/indices.h"
#include "math-search.h"
#include "rank.h"
#include "print-search-results.h"

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator  ms_merger_iterator
#define merger_set_iter_next ms_merger_iter_next
#define merger_set_iter_free ms_merger_iter_free

static void print_l2_item(struct math_l2_iter_item *item)
{
	printf("[level-2 read item] doc#%u: %.2f, occurred: ",
		item->docID, item->score);
	for (int i = 0; i < item->n_occurs; i++) {
		printf("@%u ", item->occur[i]);
	}
	printf("\n");
}

int main()
{
	/*
	 * Open index
	 */
	struct indices indices;
	char indices_path[] = "../indexerd/tmp/";
	//char indices_path[] = "/home/tk/nvme0n1/mnt-test-opt-prune-128-compact.img";

	if(indices_open(&indices, indices_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	/*
	 * Cache some index
	 */
	indices.ti_cache_limit = 0;
	indices.mi_cache_limit = 2 MB;
	// indices_cache(&indices);
	indices_print_summary(&indices);
	printf("\n");

	/*
	 * Query TeX
	 */
	//const char tex[] = "k(k+1)";
	//const char tex[] = "\\sum_{k=0}^n (k-1)k(k+1)";
	//const char tex[] = "x^2+2xy+y^2=(x+y)^2";
	const char tex[] = "\\sum_{i=0}^n x_i = x";

	/*
	 * Create level-2 inverted list
	 */
	float threshold = 0.f;
	struct math_l2_invlist *minv;
	                                        /* use same threshold */
	minv = math_l2_invlist(indices.mi, tex, &threshold, &threshold);
	if (NULL == minv) {
		prerr("cannot parse tex %s", tex);
		goto close;
	}

	/*
	 * Print participated path inverted lists
	 */
	math_qry_print(&minv->mq, 1);

	/*
	 * Create level-2 iterator
	 */
	math_l2_invlist_iter_t l2_iter = math_l2_invlist_iterator(minv);

	if (l2_iter == NULL) {
		prerr("cannot create level-2 iterator.");
		math_l2_invlist_free(minv);
		goto close;
	}

	/*
	 * Create merge set
	 */
	struct merge_set merge_set = {0};
	merge_set.iter  [0] = l2_iter;
	merge_set.upp   [0] = math_l2_invlist_iter_upp(l2_iter);
	merge_set.sortby[0] = merge_set.upp[0];
	merge_set.cur   [0] = (merger_callbk_cur) math_l2_invlist_iter_cur;
	merge_set.next  [0] = (merger_callbk_next)math_l2_invlist_iter_next;
	merge_set.skip  [0] = (merger_callbk_skip)math_l2_invlist_iter_skip;
	merge_set.read  [0] = (merger_callbk_read)math_l2_invlist_iter_read;
	merge_set.n = 1;

	printf("level-2 inverted list upperbound: %.2f\n\n", merge_set.upp[0]);

	/*
	 * Create top-K heap
	 */
	ranked_results_t rk_res;
	priority_Q_init(&rk_res, DEFAULT_N_TOP_RESULTS);

	/*
	 * Merge
	 */
	foreach (merge_iter, merger_set, &merge_set) {
		uint64_t cur; char cmd; uint64_t jump_to;
loop_head:
		cur = merger_map_call(merge_iter, cur, 0);
		printf("\n [cur docID] " C_RED "%lu\n" C_RST, cur);

		printf("Commands:\n");
		printf("r: read current and consider it as a top-K.\n");
		printf("n: go to next.\n");
		printf("s <id>: skip to <id>.\n");
		printf("q: quit.\n");
get_char:
		cmd = getchar();

		if (cmd == 'r') {
			struct math_l2_iter_item item;
			merger_map_call(merge_iter, read, 0, &item, sizeof(item));
			print_l2_item(&item);
			printf("\n");

			if (item.score > 0) {
				if (!priority_Q_full(&rk_res) ||
					item.score > priority_Q_min_score(&rk_res)) {

					struct rank_hit *hit = malloc(sizeof *hit);
					const int sz = sizeof(uint32_t) * item.n_occurs;
					hit->docID = item.docID;
					hit->score = item.score;
					hit->n_occurs = item.n_occurs;
					hit->occur = malloc(sz);
					memcpy(hit->occur, item.occur, sz);

					priority_Q_add_or_replace(&rk_res, hit);

					/* update threshold if the heap is full */
					if (priority_Q_full(&rk_res)) {
						threshold = priority_Q_min_score(&rk_res);
						printf("threshold --> %.2f\n", threshold);
					}
				}
			}

			goto loop_head;

		} else if (cmd == 'n') {
			printf("go to the next...\n");
			/* pass through */;

		} else if (cmd == 's') {
			scanf("%lu", &jump_to);
			printf("skipping to %lu ...\n", jump_to);
			int res = merger_map_call(merge_iter, skip, 0, jump_to);

			/* remember to update merge iteration min after skip() */
			merge_iter->min = ms_merger_min(merge_iter);

			printf("[skipping res] %d\n", res);
			goto loop_head;

		} else if (cmd == 'q') {
			break;
		} else {
			goto get_char;
		}
	}

	/*
	 * Free level-2 iterator and inverted list
	 */
	math_l2_invlist_iter_free(l2_iter);
	math_l2_invlist_free(minv);

	/*
	 * Sort and print search results
	 */
	priority_Q_sort(&rk_res);
	print_search_results(&indices, &rk_res, 1);
	priority_Q_free(&rk_res);

close:
	/*
	 * Close indices
	 */
	indices_close(&indices);
	mhook_print_unfree();
	return 0;
}
