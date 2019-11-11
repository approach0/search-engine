#include <stdint.h>
#include "config.h"
#include "hashtable/u16-ht.h"
#include "hashtable/float-ht.h"

struct mnc_row {
	uint16_t q_symb;
	int      q_path_cnt;

	struct float_ht subscore;

	int      n_d_symb;
	uint16_t d_symb[MAX_MATCHED_PATHS];
};

struct mnc_scorer {
	int n_row;
	struct mnc_row row[MAX_MATCHED_PATHS];
	int tot_d_symb; /* for testing early termination */

	struct u16_ht row_map;
};

void mnc_score_init(struct mnc_scorer*);
void mnc_score_free(struct mnc_scorer*);

void mnc_score_qry_path_add(struct mnc_scorer*, uint16_t);
void mnc_score_qry_path_sort(struct mnc_scorer *mnc);

void  mnc_score_doc_path_add(struct mnc_scorer*, uint16_t, uint16_t, float);
float mnc_score_calc(struct mnc_scorer*, struct mnc_scorer*);
void  mnc_score_doc_reset(struct mnc_scorer*);
