#include "search.h"

ranked_results_t
indices_run_query(struct indices* indices, struct query* qry)
{
	ranked_results_t rk_res;
	priority_Q_init(&rk_res, RANK_SET_DEFAULT_VOL);

	for (int i = 0; i < qry->len; i++) {
		const char *kw = query_get_keyword(qry, i);
		printf("%s\n", kw);
	}

	priority_Q_sort(&rk_res);
	return rk_res;
}
