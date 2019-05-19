#include <stdio.h>
#include <string.h>

#include "wstring/wstring.h"
#include "term-index/term-index.h" /* for position_t */
#include "indexer/index.h"

#include "config.h"
#include "proximity.h"
#include "query.h"

LIST_DEF_FREE_FUN(query_list_free, struct query_keyword, ln, free(p));

struct query query_new()
{
	struct query qry;
	LIST_CONS(qry.keywords);
	qry.len = 0;
	qry.n_math = 0;
	qry.n_term = 0;

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

	//fprintf(fh, "[%u]: `%S'", kw->pos, kw->wstr);
	fprintf(fh, "`%S'", kw->wstr);

	if (kw->type == QUERY_KEYWORD_TEX)
		fprintf(fh, " (tex)");

	fprintf(fh, "\n");

	LIST_GO_OVER;
}

void query_print(struct query qry, FILE* fh)
{
	list_foreach(&qry.keywords, &qry_keyword_print, fh);
//	fprintf(fh, "query length = %u (math) + %u (term) = %u.",
//	        qry.n_math, qry.n_term, qry.len);
}

struct query_keyword_arg {
	struct query_keyword *kw;
	int      target;
	int      cur;
};

static LIST_IT_CALLBK(get_query_keyword)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(arg, struct query_keyword_arg, pa_extra);

	if (arg->cur == arg->target) {
		arg->kw = kw;
		return LIST_RET_BREAK;
	}

	arg->cur += 1;
	LIST_GO_OVER;
}

struct query_keyword *query_keyword(struct query *qry, int idx)
{
	struct query_keyword_arg arg = {NULL, idx, 0};
	list_foreach(&qry->keywords, &get_query_keyword, &arg);

	return arg.kw;
}

void query_push_keyword(struct query *qry, const struct query_keyword* kw)
{
	struct query_keyword *copy;

	if (kw->type == QUERY_KEYWORD_TEX) {
		qry->n_math ++;
	} else if (kw->type == QUERY_KEYWORD_TERM) {
		qry->n_term ++;
	} else {
		fprintf(stderr, "not adding keyword due to bad type.\n");
		return;
	}

	copy = malloc(sizeof(struct query_keyword));
	memcpy(copy, kw, sizeof(struct query_keyword));
	LIST_NODE_CONS(copy->ln);

	if (copy->type == QUERY_KEYWORD_TERM)
		eng_to_lower_case_w(copy->wstr, wstr_len(copy->wstr));

	list_insert_one_at_tail(&copy->ln, &qry->keywords, NULL, NULL);
	copy->pos = (qry->len ++);
}

void
query_push_keyword_str(struct query *qry,
	const char *utf8_kw, enum query_kw_type type)
{
	struct query_keyword keyword;
	keyword.type = type;
	wstr_copy(keyword.wstr, mbstr2wstr(utf8_kw));
	query_push_keyword(qry, &keyword);
}

static struct query *adding_qry = NULL;
static int add_into_qry(struct lex_slice *slice)
{
	struct query_keyword kw;
	kw.type = QUERY_KEYWORD_TERM;
	eng_to_lower_case(slice->mb_str, strlen(slice->mb_str));
	wstr_copy(kw.wstr, mbstr2wstr(slice->mb_str));

	if (adding_qry)
		query_push_keyword(adding_qry, &kw);

	return 0;
}

void
query_digest_utf8txt(struct query *qry, const char* txt)
{
	FILE *text_fh;

	/* register lex handler  */
	g_lex_handler = add_into_qry;

	/* set static qry pointer */
	adding_qry = qry;

	/* invoke lexer */
	text_fh = fmemopen((void *)txt, strlen(txt), "r");
	lex_eng_file(text_fh);

	/* close file handler */
	fclose(text_fh);
}
