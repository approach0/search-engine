#include <stdio.h>
#include <stdlib.h>

#include "term-index/term-index.h" /* for doc_id_t */
#include "rank.h"

void test_print_res(struct rank_hit* hit, uint32_t cnt, void*arg)
{
	printf("#%u: doc#%u (score=%f)\n", cnt, hit->docID, hit->score);
}

void test_add(struct priority_Q *Q, doc_id_t id, float score)
{
	struct rank_hit *hit;

	printf("inserting score=%.3f...\n", score);

	if (!priority_Q_full(Q) ||
	    score > priority_Q_min_score(Q)) {

		hit = malloc(sizeof(struct rank_hit));
		hit->docID = id;
		hit->score = score;
		hit->n_occurs = 0;
		hit->occurs = NULL;

		if (priority_Q_add_or_replace(Q, hit))
			printf("inserted.\n");
		else
			printf("replaced.\n");
	} else {
		printf("skipped.\n");
	}

	priority_Q_print(Q);
}

int main(void)
{
	struct priority_Q  queue;
	struct rank_window window;
	uint32_t           page, tot_pages;
	const uint32_t     res_per_page = 3;
	const uint32_t     max_num_res = 5;

	priority_Q_init(&queue, max_num_res);

	test_add(&queue, 1, 2.3f);
	test_add(&queue, 2, 14.1f);
	test_add(&queue, 3, -3.8f);
	test_add(&queue, 4, 21.f);
	test_add(&queue, 5, 10.3f);
	test_add(&queue, 6, 1.1f);
	test_add(&queue, 7, -9.9f);

	printf("sorting...\n");
	priority_Q_sort(&queue);
	priority_Q_print(&queue);

	page = 0;
	do {
		printf("page#%u:\n", page + 1);
		window = rank_window_calc(&queue, page, res_per_page, &tot_pages);
		rank_window_foreach(&window, &test_print_res, NULL);
		page ++;
	} while (page < tot_pages);

	priority_Q_free(&queue);
	return 0;
}
