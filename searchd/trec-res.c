#include "trec-res.h"

static FILE *fh_trec_output = NULL;

static void log_trec_res(struct rank_result* res, void* args)
{
	P_CAST(indices, struct indices, args);
	struct rank_hit *hit = res->hit;

	size_t url_sz;
	char *url = get_blob_txt(indices->url_bi, hit->docID, 0, &url_sz);
	
	/* format: _QRY_ID_ docID docIDString rank score runID */
	fprintf(fh_trec_output, "_QRY_ID_ %u %s %u %f %s\n",
		hit->docID, url, res->cnt + 1, hit->score, "APPROACH0");
	free(url);
}

int search_results_trec_log(ranked_results_t *rk_res, struct indices *indices)
{
	struct rank_wind wind;
	int tot_pages;
	
	fh_trec_output = fopen("trec-format-results.tmp", "w");
	if (fh_trec_output == NULL)
		return 1;

	/* calculate total pages */
	wind = rank_wind_calc(rk_res, 0, DEFAULT_RES_PER_PAGE, &tot_pages);
	if (wind.to <= 0) {
		/* not valid calculation, return error */
		fclose(fh_trec_output);
		return 1;
	}

	for (int i = 0; i < tot_pages; i++) {
		wind = rank_wind_calc(rk_res, i, DEFAULT_RES_PER_PAGE, &tot_pages);
		rank_wind_foreach(&wind, &log_trec_res, indices);
	}

	fclose(fh_trec_output);
	return 0;
}
