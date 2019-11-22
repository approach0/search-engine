#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "rank.h"

void test_print_res(struct rank_result* res, void*arg)
{
	printf("[%d,%d] #%d: doc#%u (score=%f)\n", res->from, res->to, res->cnt,
		res->hit->docID, res->hit->score);
}

void test_add(struct priority_Q *Q, uint32_t id, float score)
{
	struct rank_hit *hit;

	printf("inserting score=%.3f...\n", score);

	if (!priority_Q_full(Q) ||
	    score > priority_Q_min_score(Q)) {

		hit = malloc(sizeof(struct rank_hit));
		hit->docID = id;
		hit->score = score;
		hit->n_occurs = 0;
		hit->occur = NULL;

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
	struct rank_wind   window;
	int                page, tot_pages;
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
		printf("page#%d:\n", page + 1);
		window = rank_wind_calc(&queue, page, res_per_page, &tot_pages);
		rank_wind_foreach(&window, &test_print_res, NULL);
		page ++;
	} while (page < tot_pages);

	priority_Q_free(&queue);

	mhook_print_unfree();
	return 0;
}
