#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "indices/indices.h"

#include "postlist-codec/postlist-codec.h"
#include "postlist/postlist.h"
#include "postlist/postlist.h"
#include "postlist/term-postlist.h"
#include "postlist-cache/term-postlist-cache.h"

#include "postmerge/postmerger.h"
#include "postmerge/postcalls.h"

#include "config.h"
#include "term-search.h"

void term_query_preprocess(struct term_qry_struct *tqs, int n)
{
}

int
text_qry_prepare(struct indices *indices, char *kw_str,
                 struct term_qry_struct *tqs)
{
	struct postlist_cache ci = indices->ci;
	void *po = term_postlist_cache_find(ci.term_cache, kw_str);

	term_id_t term_id = term_lookup(indices->ti, kw_str);

	if (term_id == 0) {
		/* this term is not found in dictionary */
		return 1;
	}

	if (po) {
		tqs->po = term_memo_postlist(po);
	} else {
		po = term_index_get_posting(indices->ti, term_id);
		tqs->po = term_disk_postlist(po);
	}

	tqs->term_id = term_id;
	tqs->qf = 1;
	tqs->df = term_index_get_df(indices->ti, term_id);
	return 0;
}

///*
// * query related functions
// */
//static LIST_IT_CALLBK(set_kw_values)
//{
//	LIST_OBJ(struct query_keyword, kw, ln);
//	P_CAST(indices, struct indices, pa_extra);
//	term_id_t term_id;
//
//	if (kw->type == QUERY_KEYWORD_TEX) {
//		/*
//		 * currently math expressions do not have cached posting
//		 * list, so set post_id to zero as a mark so that they
//		 * will not be deleted by query_uniq_by_post_id().
//		 */
//
//		kw->post_id = 0;
//		kw->df = 0;
//	} else if (kw->type == QUERY_KEYWORD_TERM) {
//		term_id = term_lookup(indices->ti, wstr2mbstr(kw->wstr));
//		kw->post_id = (int64_t)term_id;
//
//		if (term_id == 0)
//			kw->df = 0;
//		else
//			kw->df = (uint64_t)term_index_get_df(indices->ti,
//			                                     term_id);
//	} else {
//		assert(0);
//	}
//
//	LIST_GO_OVER;
//}
//
//void set_keywords_val(struct query *qry, struct indices *indices)
//{
//	list_foreach((list*)&qry->keywords, &set_kw_values, indices);
//}
//
//	/* sort query, to prioritize keywords in highlight stage */
//	query_sort_by_df(qry);
//
//	/* make query unique by post_id, avoid mem-posting overlap */
//	query_uniq_by_post_id(qry);
