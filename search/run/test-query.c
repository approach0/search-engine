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

#define PUSH_TERM(_str, _df, _id) \
	kw.type = QUERY_KEYWORD_TERM; \
	_PUSH(_str)

#define PUSH_TEX(_str) \
	kw.type = QUERY_KEYWORD_TEX; \
	_PUSH(_str)

int main()
{
	struct query qry = query_new();
	struct query_keyword kw;

	PUSH_TERM("dog", 90800, 1234);
	PUSH_TERM("cat", 1300, 1235);
	PUSH_TEX("f(x)");
	PUSH_TERM("dog", 999, 1234);
	PUSH_TEX("2x^3 + x");

	query_digest_utf8txt(&qry, "dog eats cat");

	query_print(qry, stdout);
	printf("\n");

	query_delete(qry);

	mhook_print_unfree();
	return 0;
}
