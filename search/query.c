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
	else
		fprintf(fh, " (df=%lu)", kw->df);

	fprintf(fh, " (post ID = %ld)", kw->post_id);
	fprintf(fh, "\n");

	LIST_GO_OVER;
}

void query_print_to(struct query qry, FILE* fh)
{
	list_foreach(&qry.keywords, &qry_keyword_print, fh);
	fprintf(fh, "query length = %u (math) + %u (term) = %u.",
	        qry.n_math, qry.n_term, qry.len);
}

struct query_keyword_arg {
	wchar_t *wstr;
	int      target;
	int      cur;
};

static LIST_IT_CALLBK(get_query_keyword)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(arg, struct query_keyword_arg, pa_extra);

	if (arg->cur == arg->target) {
		arg->wstr = kw->wstr;
		return LIST_RET_BREAK;
	}

	arg->cur += 1;
	LIST_GO_OVER;
}

wchar_t *query_keyword(struct query qry, int idx)
{
	struct query_keyword_arg arg = {NULL, idx, 0};
	list_foreach(&qry.keywords, &get_query_keyword, &arg);

	return arg.wstr;
}

char *query_get_keyword(struct query *qry, int idx)
{
	struct query_keyword_arg arg = {NULL, idx, 0};
	list_foreach(&qry->keywords, &get_query_keyword, &arg);

	return wstr2mbstr(arg.wstr);
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
	kw.df   = 0;
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

/*
 * query sort
 */
static
LIST_CMP_CALLBK(compare)
{
	uint64_t df0, df1;
	struct query_keyword *p0, *p1;
	p0 = MEMBER_2_STRUCT(pa_node0, struct query_keyword, ln);
	p1 = MEMBER_2_STRUCT(pa_node1, struct query_keyword, ln);

	if (p0->type == QUERY_KEYWORD_TEX)
		df0 = 0;
	else
		df0 = p0->df;

	if (p1->type == QUERY_KEYWORD_TEX)
		df1 = 0;
	else
		df1 = p1->df;

	return df0 < df1;
}

void query_sort_by_df(const struct query *qry)
{
	struct list_sort_arg sort;
	sort.cmp = &compare;
	sort.extra = NULL;

	list_sort((list*)&qry->keywords, &sort);
}

/*
 * query uniq
 */
struct dele_identical_kw_arg {
	int64_t           post_id;
	struct list_node *stop_at;
	struct query     *query;
};

static
LIST_IT_CALLBK(dele_identical_kw)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(arg, struct dele_identical_kw_arg, pa_extra);
	bool res;

	if (pa_now->now == arg->stop_at)
		res = LIST_RET_BREAK;
	else
		res = LIST_RET_CONTINUE;

	if (kw->post_id == arg->post_id) {
		list_detach_one(pa_now->now, pa_head, pa_now, pa_fwd);

		arg->query->len --;

		if (kw->type == QUERY_KEYWORD_TEX)
			arg->query->n_math --;
		else if (kw->type == QUERY_KEYWORD_TERM)
			arg->query->n_term --;
		else
			fprintf(stderr, "bad keyword type!\n");

		free(kw);
	}

	return res;
}

static
LIST_IT_CALLBK(uniq_kw)
{
	LIST_OBJ(struct query_keyword, kw, ln);
	P_CAST(query, struct query, pa_extra);

	struct dele_identical_kw_arg arg;
	struct list_it sub_li;

	/* if we do not need to uniq this element? (post_id = 0)
	 * or this is the last element? */
	if (kw->post_id != 0 &&
	    pa_now->now != pa_head->last) {

		/* set arguments for dele_identical_kw() */
		arg.post_id = kw->post_id;
		arg.stop_at = pa_head->last;
		arg.query   = query;

		/* for each sublist after "this element" */
		sub_li = list_get_it(pa_now->now->next);
		list_foreach(&sub_li, &dele_identical_kw, &arg);

		/* update iterator */
		*pa_head = list_get_it(pa_head->now);
		*pa_fwd = list_get_it(pa_now->now->next);
	}

	LIST_GO_OVER;
}

void query_uniq_by_post_id(struct query* qry)
{
	list_foreach(&qry->keywords, &uniq_kw, qry);
}
