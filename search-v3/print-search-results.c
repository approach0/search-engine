#include <stdio.h>
#include <stdlib.h>
#include "indices-v3/indices.h"
#include "config.h" /* for DEFAULT_RES_PER_PAGE */
#include "txt2snippet.h"
#include "rank.h"

static void print_res_item(struct rank_result *res, void* arg)
{
	char *txt; size_t txt_sz;
	list highlight_list = LIST_NULL;
	struct rank_hit *hit = res->hit;
	PTR_CAST(indices, struct indices, arg);

	printf("result[%d]: doc#%u score=%.3f\n",
	        res->cnt, hit->docID, hit->score);

	/* get URL */
	txt = get_blob_txt(indices->url_bi, hit->docID, 0, &txt_sz);
	printf("URL: %s\n", txt);
	free(txt);

	{ /* print occur positions */
		int i;
		printf("Occurs: ");
		for (i = 0; i < hit->n_occurs; i++)
			printf("%u ", hit->occur[i]);
		printf("\n");
	}

	/* get document text */
	txt = get_blob_txt(indices->txt_bi, hit->docID, 1, &txt_sz);

	/* prepare highlighter arguments */
	highlight_list = txt2snippet(txt, txt_sz, hit->occur, hit->n_occurs);
	free(txt);

	/* print snippet */
	snippet_hi_print(&highlight_list);

	/* free highlight list */
	snippet_free_highlight_list(&highlight_list);
	printf("\n\n");
}

void print_search_results(struct indices *indices,
	ranked_results_t *rk_res, int page)
{
	struct rank_wind wind;
	int i, from_page = page, to_page = page;
	int tot_pages;
	wind = rank_wind_calc(rk_res, 0, DEFAULT_RES_PER_PAGE, &tot_pages);

	/* show all pages */
	if (page < 1) {
		from_page = 1;
		to_page = tot_pages;
	}

	for (i = from_page - 1; i < MIN(to_page, tot_pages); i++) {
		printf("page %d/%u, top result(s) from %u to %u:\n",
			i + 1, tot_pages, wind.from + 1, wind.to);

		wind = rank_wind_calc(rk_res, i, DEFAULT_RES_PER_PAGE, &tot_pages);
		rank_wind_foreach(&wind, &print_res_item, indices);
	}
}
