#include "common/common.h"
#include "tex-parser/head.h"
#include "mnc-score.h"

void mnc_score_init(struct mnc_scorer *mnc)
{
	mnc->n_row = 0;
	mnc->row_map = u16_ht_new(0);

	mnc->cross = u16_ht_new(0);
}

void mnc_score_free(struct mnc_scorer *mnc)
{
	for (int i = 0; i < mnc->n_row; i++) {
		struct mnc_row *row = mnc->row + i;
		float_ht_free(&row->subscore);
	}

	u16_ht_free(&mnc->row_map);
	u16_ht_free(&mnc->cross);
}

void  mnc_score_doc_reset(struct mnc_scorer *mnc)
{
	for (int i = 0; i < mnc->n_row; i++) {
		struct mnc_row *row = mnc->row + i;
		float_ht_reset(&row->subscore, 0);
		row->n_d_symb = 0;
	}
}

/* insert mnc[q] */
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

/* sort mnc[q] by q_path_cnt */
void mnc_score_qry_path_sort(struct mnc_scorer *mnc)
{
	/* TODO: use quick-sort? */
	for (int i = 0; i < mnc->n_row; i++) {
		struct mnc_row *row_i = mnc->row + i;
		for (int j = i + 1; j < mnc->n_row; j++) {
			struct mnc_row *row_j = mnc->row + j;
			if (row_i->q_path_cnt < row_j->q_path_cnt) {
				/* swap */
				struct mnc_row tmp = *row_i;
				*row_i = *row_j;
				*row_j = tmp;
				/* update lookup table entries */
				u16_ht_update(&mnc->row_map, row_i->q_symb, i);
				u16_ht_update(&mnc->row_map, row_j->q_symb, j);
			}
		}
	}
}

/* accumulate score `s' to mnc[q][d] */
void
mnc_score_doc_path_add(struct mnc_scorer *mnc, uint16_t q, uint16_t d, float s)
{
	/* row lookup */
	int idx = u16_ht_lookup(&mnc->row_map, q);
	if (unlikely(-1 == idx)) {
		prerr("unlisted query symbol");
		return;
	}

	struct mnc_row *row = mnc->row + idx;

	/* column lookup */
	float subscore = float_ht_lookup(&row->subscore, d);
	if (subscore == -1.f) {
		row->d_symb[row->n_d_symb] = d;
		row->n_d_symb += 1;
	}

	/* accumulate score */
	float_ht_incr(&row->subscore, d, s);
}

/* get greedily aligned pairs of document/query symbols */
float mnc_score_align(struct mnc_scorer* mnc)
{
	float score = 0;

	/* use temporary variables in mnc_scorer structure */
	mnc->n_cross = 0;
	u16_ht_reset(&mnc->cross, 0);

	/* for each row, find out the unmatched pair of symbols w/ highest score */
	for (int i = 0; i < mnc->n_row; i++) {
		struct mnc_row *row = mnc->row + i;
		float max_subscore = 0.f;
		uint16_t max_d = S_NIL;

		for (int j = 0; j < row->n_d_symb; j++) {
			uint16_t d = row->d_symb[j];
			if (-1 != u16_ht_lookup(&mnc->cross, d))
				continue; /* if it is already matched, skip */

			/* if it has the highest score? */
			float subscore = float_ht_lookup(&row->subscore, d);
			if (max_subscore < subscore) {
				max_subscore = subscore;
				max_d = d;
			}
		}

		/* save matched/paired document symbol for this row */
		mnc->paired_d[i] = max_d;

		/* skip accumulating score if there is no match in this row */
		if (max_d == S_NIL)
			continue;

		/* make record of already matched */
		u16_ht_incr(&mnc->cross, max_d, 1);
		mnc->n_cross += 1;

		/* accumulate score */
		score += max_subscore;
	}

	return score;
}

/* calculate symbol score for a local mnc given "global" aligned mnc */
float mnc_score_calc(struct mnc_scorer* gmnc, struct mnc_scorer* lmnc)
{
	float score = 0;

	/* in already aligned rows of global mnc */
	for (int i = 0; i < gmnc->n_cross; i++) {
		/* q, d are aligned pair of this row */
		uint16_t q = gmnc->row[i].q_symb;
		uint16_t d = gmnc->paired_d[i];

		/* if this query symbol has any matched pair in local mnc */
		int idx = u16_ht_lookup(&lmnc->row_map, q);
		if (-1 == idx) /* No? Skip. */
			continue;

		/* get matched score from local mnc */
		struct mnc_row *row = lmnc->row + idx;
		float subscore = float_ht_lookup(&row->subscore, d);
		if (subscore > 0)
			score += subscore;
	}

	return score;
}
