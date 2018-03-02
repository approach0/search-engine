#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "search/postmerge.h"
#include "search/math-expr-search.h"

static void
math_posting_on_merge(uint64_t cur_min, struct postmerge* pm,
                      void* extra_args)
{
	P_CAST(mesa, struct math_extra_score_arg, extra_args);
	P_CAST(touch, int, mesa->expr_srch_arg);
	struct math_posting_item *po_item = pm->cur_pos_item[0];
	printf("HIT: doc#%u, exp#%u.\n", po_item->doc_id, po_item->exp_id);
	*touch = 1;
}

int
math_qac_index_uniq_tex(math_index_t mi, const char *tex,
                        doc_id_t docID, exp_id_t expID)
{
	struct tex_parse_ret parse_ret;
	int touch = 0;

	math_expr_search(mi, (char *)tex, MATH_SRCH_EXACT_STRUCT,
	                 &math_posting_on_merge, &touch);
	if (touch) {
		printf("Identical expression exists in index.\n");
		return 1;
	}

	parse_ret = tex_parse(tex, 0, false);

	if (parse_ret.code != PARSER_RETCODE_ERR) {
		//subpaths_print(&parse_ret.subpaths, stdout);
		printf("Adding expression into index...\n");
		math_index_add_tex(mi, docID, expID, parse_ret.subpaths);
		subpaths_release(&parse_ret.subpaths);
	} else {
		printf("parser error: %s\n", parse_ret.msg);
	}

	return 0;
}
