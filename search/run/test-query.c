#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "wstring/wstring.h"
#include "indexer/index.h"

#include "config.h"
#include "query.h"

#define _PUSH(_str) \
	wstr_copy(kw.wstr, L ## _str); \
	query_push_keyword(&qry, &kw)

#define PUSH_TERM(_str, _df, _post_id) \
	kw.type = QUERY_KEYWORD_TERM; \
	kw.df = _df; \
	kw.post_id = _post_id; \
	_PUSH(_str)

#define PUSH_TEX(_str) \
	kw.type = QUERY_KEYWORD_TEX; \
	kw.df = 0; \
	kw.post_id = 0; \
	_PUSH(_str)

int main()
{
	struct query qry = query_new();
	struct query_keyword kw;

	PUSH_TERM("dog", 90800, 1234);
	PUSH_TERM("你好", 1300, 1235);
	PUSH_TEX("f(x)");
	PUSH_TERM("the", 9999999, 1234);
	PUSH_TEX("2x^3 + x");

	query_digest_utf8txt(&qry, lex_eng_file, "hello world");

	printf("before sorting: ");
	query_print_to(qry, stdout);
	printf("\n");

	query_sort_by_df(&qry);

	printf("after sorting: \n");
	query_print_to(qry, stdout);
	printf("\n");

	query_uniq_by_post_id(&qry);

	printf("after uniq: \n");
	query_print_to(qry, stdout);
	printf("\n");

	query_delete(qry);

	mhook_print_unfree();
	return 0;
}
