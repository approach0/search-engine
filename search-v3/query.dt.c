#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/common.h"
#include "wstring/wstring.h"
#include "txt-seg/lex.h"
#include "indices-v3/config.h" /* for INDICES_TXT_LEXER */
#include "query.h"

void query_delete(struct query qry)
{
	li_free(qry.keywords, struct query_keyword, ln, free(e));
}

void query_print_kw(struct query_keyword *kw, FILE* fh)
{
	if (kw == NULL) {
		fprintf(fh, "invalid pointer\n");
		return;
	}

	switch (kw->op) {
	case QUERY_OP_OR:
		fprintf(fh, "[ OR] ");
		break;
	case QUERY_OP_AND:
		fprintf(fh, "[AND] ");
		break;
	case QUERY_OP_NOT:
		fprintf(fh, "[NOT] ");
		break;
	default:
		fprintf(fh, "[NIL] ");
	}

	fprintf(fh, "`%S' ", kw->wstr);
	if (kw->type == QUERY_KW_TEX)
		fprintf(fh, "(tex) ");
	fprintf(fh, "\n");
}

void query_print(struct query qry, FILE* fh)
{
	fprintf(fh, "[query] %u math(s) + %u term(s), total %u.\n",
	        qry.n_math, qry.n_term, qry.len);

	int cnt = 0;
	foreach (iter, li, qry.keywords) {
		struct query_keyword *kw = li_entry(kw, iter->cur, ln);
		fprintf(fh, "(%d) ", cnt++);
		query_print_kw(kw, fh);
	}
}

int query_push_kw(struct query *qry, const char *utf8_kw,
	enum query_kw_type type, enum query_kw_bool_op op)
{
	/* update stats */
	if (type != QUERY_KW_TEX && type != QUERY_KW_TERM) {
		prerr("Not adding keyword due to bad type.");
		return 1;
	}

	/* push new keyword */
	struct query_keyword *kw = li_new_entry(kw, ln);

	if (kw == NULL) {
		prerr("Not adding keyword due to malloc failure.");
		return 2;
	}

	kw->type = type;
	kw->op   = op;
	wstr_copy(kw->wstr, mbstr2wstr(utf8_kw));

	li_append(&qry->keywords, &kw->ln);

	/* convert to lowercase for all term keywords */
	if (type == QUERY_KW_TERM) {
		eng_to_lower_case_w(kw->wstr, wstr_len(kw->wstr));
		qry->n_term ++;
	} else {
		qry->n_math ++;
	}

	qry->len ++;
	return 0;
}

struct query_keyword *query_get_kw(struct query* qry, int idx)
{
	int cnt = 0;
	foreach (iter, li, qry->keywords) {
		struct query_keyword *kw = li_entry(kw, iter->cur, ln);
		if (cnt == idx) {
			li_iter_free(iter);
			return kw;
		}
		cnt += 1;
	}

	return NULL;
}

static int digest_handler(struct lex_slice*);

static struct query *digest_qry = NULL;
static int digest_fails = 0;

int query_digest_txt(struct query *qry, const char* utf8_txt)
{
	FILE *text_fh;

	/* register lex handler  */
	g_lex_handler = digest_handler;

	/* set static parameters to be passed into lexer */
	digest_qry = qry;
	digest_fails = 0;

	/* invoke lexer */
	text_fh = fmemopen((void *)utf8_txt, strlen(utf8_txt), "r");

	INDICES_TXT_LEXER(text_fh);

	/* close file handler */
	fclose(text_fh);

	return digest_fails;
}

static int digest_handler(struct lex_slice *slice)
{
	char str[MAX_QUERY_BYTES];

	strcpy(str, slice->mb_str);
	eng_to_lower_case(str, strlen(str));

	if (NULL == digest_qry || query_push_kw(digest_qry, str,
	    QUERY_KW_TERM, QUERY_OP_OR))
		digest_fails ++;

	return 0;
}
