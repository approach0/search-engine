#include "mhook/mhook.h"
#include "config.h"
#include "math-search.h"

/* use MaxScore merger */
#define merger_set_iterator  ms_merger_iterator
#define merger_set_iter_next ms_merger_iter_next
#define merger_set_iter_free ms_merger_iter_free

static void keyprint(uint64_t k)
{
	printf("#%u, #%u, r%u", key2doc(k), key2exp(k), key2rot(k));
}

static void print_l2_item(struct math_l2_iter_item *item)
{
	printf("doc#%u: %.2f, occurred: ", item->docID, item->score);
	for (int i = 0; i < item->n_occurs; i++) {
		printf("@%u ", item->occur[i]);
	}
	printf("\n");
}

int main()
{
	math_index_t mi = math_index_open("../math-index-v3/tmp", "r");
	if (mi == NULL) {
		prerr("cannot open index.");
		return 1;
	}

	const char tex[] = "k(b+a)+ab+\\sqrt{b}+k+a";
	float threshold = 0.f;
	struct math_l2_invlist *minv = math_l2_invlist(mi, tex, &threshold);

	if (NULL == minv) {
		prerr("cannot parse tex %s", tex);
		math_index_close(mi);
		return 2;
	}

	struct merge_set merge_set = {0};
	merge_set.iter[0] = math_l2_invlist_iterator(minv);
	merge_set.upp [0] = 123.f;
	merge_set.cur [0] =  (merger_callbk_cur)math_l2_invlist_iter_cur;
	merge_set.next[0] = (merger_callbk_next)math_l2_invlist_iter_next;
	merge_set.skip[0] = (merger_callbk_skip)NULL;
	merge_set.read[0] = (merger_callbk_read)math_l2_invlist_iter_read;
	merge_set.n = 1;

	if (merge_set.iter[0] == NULL) {
		prerr("cannot create level-2 iterator.");
		goto end;
	}

	foreach (merge_iter, merger_set, &merge_set) {
		printf("=== Iteration ===\n");
		struct math_l2_iter_item item;
		merger_map_call(merge_iter, read, 0, &item, sizeof(item));
		print_l2_item(&item);

		P_CAST(l2_iter, struct math_l2_invlist_iter, merge_iter->set.iter[0]);
		math_pruner_print(l2_iter->pruner);
		ms_merger_iter_print(merge_iter, keyprint);
		printf("\n");

	}

	math_l2_invlist_iter_free(merge_set.iter[0]);

end:
	math_l2_invlist_free(minv);
	math_index_close(mi);

	mhook_print_unfree();
	return 0;
}
