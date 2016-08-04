#include <stdio.h>
#include <stdlib.h>

#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"
#include "wstring/wstring.h"

#include "config.h"
#include "query.h"

#define _PUSH(_str) \
	wstr_copy(kw.wstr, L ## _str); \
	query_push_keyword(&qry, &kw)

#define PUSH_TERM(_str, _df) \
	kw.type = QUERY_KEYWORD_TERM; \
	kw.df = _df; \
	_PUSH(_str)

#define PUSH_TEX(_str) \
	kw.type = QUERY_KEYWORD_TEX; \
	kw.df = 123; \
	_PUSH(_str)

int main()
{
	struct query qry = query_new();
	struct query_keyword kw;

	text_segment_init("");

	PUSH_TERM("dog", 90800);
	PUSH_TERM("你好", 1300);
	PUSH_TEX("f(x)");
	PUSH_TERM("the", 9999999);
	PUSH_TEX("2x^3 + x");

	query_digest_utf8txt(&qry, "hello world");

	printf("before sorting:\n");
	query_print_to(qry, stdout);

	query_sort(&qry);

	printf("after sorting:\n");
	query_print_to(qry, stdout);

	query_delete(qry);
	text_segment_free();
	return 0;
}
