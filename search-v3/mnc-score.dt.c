#include "common/common.h"
#include "mnc-score.h"

void mnc_score_init(struct mnc_scorer *mnc)
{
	mnc->n_row = 0;
	mnc->tot_d_symb = 0;
	mnc->row_map = u16_ht_new(0);
}

void mnc_score_free(struct mnc_scorer *mnc)
{
	u16_ht_free(&mnc->row_map);

	for (int i = 0; i < mnc->n_row; i++) {
		struct mnc_row *row = mnc->row + i;
		float_ht_free(&row->subscore);
	}
}

void  mnc_score_doc_reset(struct mnc_scorer *mnc)
{
	for (int i = 0; i < mnc->n_row; i++) {
		struct mnc_row *row = mnc->row + i;
		float_ht_reset(&row->subscore, 0);
		row->n_d_symb = 0;
	}
	mnc->tot_d_symb = 0;
}

void mnc_score_qry_path_add(struct mnc_scorer *mnc, uint16_t q)
{
	struct mnc_row *row;
	int idx = u16_ht_lookup(&mnc->row_map, q);

	if (idx == -1) {
		row = mnc->row + mnc->n_row;

		row->q_symb     = q;
		row->q_path_cnt = 1;
		row->subscore   = float_ht_new(0);
		row->n_d_symb   = 0;

		u16_ht_update(&mnc->row_map, q, mnc->n_row);

		mnc->n_row ++;
	} else {
		row = mnc->row + idx;

		row->q_path_cnt += 1;
	}
}

void mnc_score_qry_path_sort(struct mnc_scorer *mnc)
{
	for (int i = 0; i < mnc->n_row; i++) {
		struct mnc_row *row_i = mnc->row + i;
		for (int j = i + 1; j < mnc->n_row; j++) {
			struct mnc_row *row_j = mnc->row + j;
			if (row_i->q_path_cnt < row_j->q_path_cnt) {
				struct mnc_row tmp = *row_i;
				*row_i = *row_j;
				*row_j = tmp;

				u16_ht_update(&mnc->row_map, row_i->q_symb, i);
				u16_ht_update(&mnc->row_map, row_j->q_symb, j);
			}
		}
	}
}

void
mnc_score_doc_path_add(struct mnc_scorer *mnc, uint16_t q, uint16_t d, float s)
{
	int idx = u16_ht_lookup(&mnc->row_map, q);
	if (-1 == idx) {
		prerr("unlisted query symbol");
		return;
	}

	struct mnc_row *row = mnc->row + idx;

	float subscore = float_ht_lookup(&row->subscore, d);
	if (subscore == -1.f) {
		row->d_symb[row->n_d_symb] = d;
		row->n_d_symb += 1;
		mnc->tot_d_symb += 1;
	}

	float_ht_incr(&row->subscore, d, s);
}

float mnc_score_calc(struct mnc_scorer* gmnc, struct mnc_scorer* lmnc)
{
	return 0.f;
}
