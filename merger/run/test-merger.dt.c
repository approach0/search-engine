#include <stdio.h>
#include <stdlib.h>
#include "mhook/mhook.h"
#include "mergers.h"
#include "example.h"

/* utilities to initialize our example inverted list */
int arr_len(int *a)
{
	int i = 0;
	while (a[i++] != 0);
	return i - 1;
}

float max(float *a, int l)
{
	float max = 0;
	for (int i = 0; i < l; i++)
		if (a[i] > max)
			max = a[i];
	return max;
}

#define INV_INIT(_id, _score) \
	((struct example_invlist) { \
		arr_len(_id), _id, _score, max(_score, arr_len(_id)) \
	})

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator  ms_merger_iterator
#define merger_set_iter_next ms_merger_iter_next
#define merger_set_iter_free ms_merger_iter_free

int main()
{
	/* example data */
	int id[][1024] = {
		{3,      4,     5,      7,     8,    16,               0},
		{2,      3,     4,      5,     7,     8,    16,   17,  0},
		{1,      4,     5,      6,     8,    12,    16,   19,  0},
		{1,      2,     3,      4,     7,     8,    12,   16,  0}
	};

	float score[][1024] = {
		{4.f,  3.f,   2.2f,  2.7f,  1.8f,  4.2f},
		{1.f,  3.f,   3.1f,  2.8f,  1.1f,  0.2f,  3.1f, 2.4f},
		{1.f,  2.f,   2.1f,  2.1f,  1.5f,  1.0f,  2.1f, 2.2f},
		{2.f,  3.f,   1.f,   3.4f,  1.5f,  2.8f,  2.2f, 1.3f}
	};

	/* initialize example inverted lists and add into merge set */
	const int n = sizeof id / sizeof id[0];
	struct example_invlist invlist[n];
	struct merge_set merge_set = {0};

	for (int i = 0; i < n; i++) {
		invlist[i] = INV_INIT(id[i], score[i]);

		merge_set.iter[i] = example_invlist_iterator(&invlist[i]);
		merge_set.upp [i] = invlist[i].upper;
		merge_set.cur [i] =  (merger_callbk_cur)example_invlist_iter_cur;
		merge_set.next[i] = (merger_callbk_next)example_invlist_iter_next;
		merge_set.skip[i] = (merger_callbk_skip)example_invlist_iter_skip;
		merge_set.read[i] = (merger_callbk_read)example_invlist_iter_read;
		merge_set.n += 1;
	}

	/* Max-Score merging */
	float theta = 0.0f;
	foreach (iter, merger_set, &merge_set) {
		float doc_score = 0.f;
		for (int i = 0; i < iter->size; i++) {
			if (doc_score + iter->acc_upp[i] < theta) {
				printf("doc#%lu pruned:\n", iter->min);
				doc_score = 0.f;
				break;
			}

			/* advance those in skipping set */
			if (i > iter->pivot) {
				printf("skipping [%d]\n", iter->map[i]);
				i = ms_merger_map_follow(iter, i);
			}

			/* accumulate precise partial score */
			uint64_t cur = merger_map_call(iter, cur, i);
			if (cur == iter->min) {
				float partial_score;
				merger_map_call(iter, read, i, &partial_score, sizeof(float));
				doc_score += partial_score;
			}
		}

		/* printing */
		printf("theta = %.2f\n", theta);
		ms_merger_iter_print(iter);

		if (doc_score > 0.f) {
			/* push document into top-K and update theta */
			printf("[doc#%lu score=%.2f]\n", iter->min, doc_score);
			theta += 1.5f; /* let us just simulate */
			ms_merger_lift_up_pivot(iter, theta, no_upp_relax, NULL);
		}

		printf("\n");
	}

	for (int i = 0; i < n; i++)
		example_invlist_iter_free(merge_set.iter[i]);

	mhook_print_unfree();
	return 0;
}
