#include "wstring/wstring.h"
#include "term-index/term-index.h" /* for position_t */
#include "config.h"
#include "proximity.h"
#include "search.h"

LIST_DEF_FREE_FUN(query_list_free, struct query_keyword, ln, free(p));

struct query query_new()
{
	struct query qry;
	LIST_CONS(qry.keywords);
	qry.len = 0;
	return qry;
}

void query_delete(struct query qry)
{
	query_list_free(&qry.keywords);
	qry.len = 0;
}

static LIST_IT_CALLBK(qry_keyword_print)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(fh, FILE, pa_extra);

	fprintf(fh, "[%u]: `%S'", kw->pos, kw->wstr);

	if (kw->type == QUERY_KEYWORD_TEX)
		fprintf(fh, " (tex)");

	if (pa_now->now == pa_head->last)
		fprintf(fh, ".");
	else
		fprintf(fh, ", ");

	LIST_GO_OVER;
}

void query_print_to(struct query qry, FILE* fh)
{
	fprintf(fh, "query: ");
	list_foreach(&qry.keywords, &qry_keyword_print, fh);
	fprintf(fh, "\n");
}

void query_push_keyword(struct query *qry, const struct query_keyword* kw)
{
	struct query_keyword *copy = malloc(sizeof(struct query_keyword));

	memcpy(copy, kw, sizeof(struct query_keyword));
	LIST_NODE_CONS(copy->ln);

	list_insert_one_at_tail(&copy->ln, &qry->keywords, NULL, NULL);
	copy->pos = (qry->len ++);
}

static LIST_IT_CALLBK(add_into_qry)
{
	LIST_OBJ(struct text_seg, seg, ln);
	P_CAST(qry, struct query, pa_extra);
	struct query_keyword kw;

	kw.type = QUERY_KEYWORD_TERM;
	wstr_copy(kw.wstr, mbstr2wstr(seg->str));

	query_push_keyword(qry, &kw);
	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(txt_seg_list_free, struct text_seg,
                  ln, free(p));

void query_digest_utf8txt(struct query *qry, const char* txt)
{
	list li = text_segment(txt);
	list_foreach(&li, &add_into_qry, qry);
	txt_seg_list_free(&li);
}
