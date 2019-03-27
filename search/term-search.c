#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/common.h"
#include "indices/indices.h"

#include "postlist-codec/postlist-codec.h"
#include "postlist/postlist.h"
#include "postlist/postlist.h"
#include "postlist/term-postlist.h"
#include "postlist-cache/term-postlist-cache.h"

#include "postmerge/postmerger.h"
#include "postmerge/postcalls.h"

#include "term-search.h"

static void
term_qry_struct_print(struct term_qry_struct *tqs, int n)
{
	for (int i = 0; i < n; i++) {
		printf("[%u] %s,  ", i, tqs[i].kw_str);
	}
	printf("\n");
}

static int
term_qry_struct_remove(struct term_qry_struct *tqs, int n, int j)
{
	for (int i = j; i < n - 1; i++) {
		tqs[i] = tqs[i + 1];
	}

	return n - 1;
}

int term_query_merge(struct term_qry_struct *tqs, int n)
{
#ifdef DEBUG_TERM_QUERY_STRUCT_MERGE
	printf("before merge:\n");
	term_qry_struct_print(tqs, n);
#endif

	for (int i = 0; i < n; i++)
		for (int j = i + 1; j < n; j++)
			if (( tqs[i].term_id && tqs[i].term_id == tqs[j].term_id) ||
			(!tqs[i].term_id && 0 == strcmp(tqs[i].kw_str, tqs[j].kw_str))) {
				/* merge the two entries */
				n = term_qry_struct_remove(tqs, n, j);
				tqs[i].qf ++;
#ifdef DEBUG_TERM_QUERY_STRUCT_MERGE
				printf("after rm [%u]\n", j);
				term_qry_struct_print(tqs, n);
#endif
				j --; /* redo at j */
			}

	return n;
}

int
term_qry_prepare(struct indices *indices, char *kw_str,
                 struct term_qry_struct *tqs)
{
	term_id_t term_id = term_lookup(indices->ti, kw_str);

	strcpy(tqs->kw_str, kw_str);

	if (term_id == 0) {
		/* this term is not found in dictionary */
		tqs->term_id = 0;
		return 1;
	}

	tqs->term_id = term_id;
	tqs->qf = 1;
	tqs->df = term_index_get_df(indices->ti, term_id);
	return 0;
}


struct postmerger_postlist
postmerger_term_postlist(struct indices *indices, struct term_qry_struct *tqs)
{
	struct postlist_cache ci = indices->ci;
	void *po;

	/* find it in cache first */
	po = term_postlist_cache_find(ci.term_cache, tqs->kw_str);

	if (po) {
		/* cached it, retrieve... */
		return term_memo_postlist(po);
	} else {
		/* otherwise, read from disk */
		po = term_index_get_posting(indices->ti, tqs->term_id);
		return term_disk_postlist(po);
	}
}
