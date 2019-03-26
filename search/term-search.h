#include "term-index/config.h"
#include "term-index/term-index.h"

struct term_postlist_item {
	doc_id_t doc_id;
	uint32_t tf;
	uint32_t pos_arr[MAX_TERM_INDEX_ITEM_POSITIONS];
};

struct term_qry_struct {
	uint32_t term_id, qf;
	uint32_t df;
	struct postmerger_postlist po;
};

void term_query_preprocess(struct term_qry_struct *, int);
int text_qry_prepare(struct indices*, char*, struct term_qry_struct*);
