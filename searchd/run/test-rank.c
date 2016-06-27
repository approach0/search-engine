#include <stdio.h>
#include <stdlib.h>

#include "term-index/term-index.h" /* for doc_id_t */
#include "rank.h"

void test_print_res(struct rank_set *rs, struct rank_hit* hit,
                    uint32_t cnt, void*arg)
{
	printf("#%u: doc#%u (score=%f)\n", cnt, hit->docID, hit->score);
}

#define ADD_HIT(_docID, _score) \
	rank_set_hit(&rk_set, _docID, _score); \
	rank_print(&rk_set);

int main(void)
{
	struct rank_set  rk_set;
	struct rank_wind win;
	uint32_t         page, total_pages;
	const uint32_t   res_per_page = 3;
	const uint32_t   max_num_res = 5;

	rank_set_init(&rk_set, max_num_res);

	ADD_HIT(1, 2.3f);
	ADD_HIT(2, 14.1f);
	ADD_HIT(3, -3.8f);
	ADD_HIT(4, 21.f);
	ADD_HIT(5, 10.3f);
	ADD_HIT(6, 1.1f);

	printf("sorting...\n");
	rank_sort(&rk_set);
	// rank_print(&rk_set);

	page = 0;
	do {
		printf("page#%u:\n", page + 1);
		win = rank_window_calc(&rk_set, page, res_per_page, &total_pages);
		rank_window_foreach(&win, &test_print_res, NULL);
		page ++;
	} while (page < total_pages);

	rank_set_free(&rk_set);
	return 0;
}
