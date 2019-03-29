#include "term-index/config.h"
#include "term-index/term-index.h"

#include "config.h"

struct term_qry_struct {
	uint32_t term_id, qf;
	uint32_t df;
	char   kw_str[MAX_QUERY_BYTES];
};

int term_query_merge(struct term_qry_struct*, int);
int term_qry_prepare(struct indices*, char*, struct term_qry_struct*);

struct postmerger_postlist
postmerger_term_postlist(struct indices*, struct term_qry_struct*);
