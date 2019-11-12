#include <stdint.h>
#include "config.h"
#include "hashtable/u16-ht.h"
#include "hashtable/float-ht.h"

struct mnc_row {
	uint16_t q_symb; /* query symbol */
	int      q_path_cnt; /* path count */

	/* score for each pair of query-doc symbols */
	struct float_ht subscore;

	int      n_d_symb; /* number of document paths in this row */
	uint16_t d_symb[MAX_MATCHED_PATHS]; /* document paths in this row */
};

/* "mark and cross" scorer */
struct mnc_scorer {
	int n_row; /* number of rows */
	struct u16_ht row_map; /* fast lookup table for row from symbol */
	struct mnc_row row[MAX_MATCHED_PATHS];

	/* temporary variables for saving alignment results */
	uint16_t paired_d[MAX_MATCHED_PATHS];
	struct u16_ht cross; /* aligned / matched document symbols */
	int n_cross; /* number of aligned /matched document symbols */
};

void mnc_score_init(struct mnc_scorer*);
void mnc_score_free(struct mnc_scorer*);

void mnc_score_qry_path_add(struct mnc_scorer*, uint16_t);
void mnc_score_qry_path_sort(struct mnc_scorer *mnc);

void  mnc_score_doc_path_add(struct mnc_scorer*, uint16_t, uint16_t, float);
float mnc_score_align(struct mnc_scorer*);
float mnc_score_calc(struct mnc_scorer*, struct mnc_scorer*);
void  mnc_score_doc_reset(struct mnc_scorer*);
