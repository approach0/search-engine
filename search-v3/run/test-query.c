#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "query.h"

#define PUSH(_kw, _type, _op) \
	res = query_push_kw(&qry, _kw, _type, _op); \
	printf("res=%d\n", res)

#define GET(_idx) \
	kw = query_get_kw(&qry, _idx); \
	fprintf(stderr, "get(%d): ", _idx); \
	query_print_kw(kw, stderr);

int main()
{
	struct query qry = QUERY_NEW;
	int res;
	struct query_keyword *kw;

	PUSH("dog",      QUERY_KW_TERM, QUERY_OP_OR);
	PUSH("2x^3 + x", QUERY_KW_TEX,  QUERY_OP_OR);
	PUSH("eats",     QUERY_KW_TERM, QUERY_OP_AND);
	PUSH("f(x)",     QUERY_KW_TEX,  QUERY_OP_AND);
	PUSH("Loves",    QUERY_KW_TERM, QUERY_OP_NOT);
	PUSH("2x^2+2x",  QUERY_KW_TEX,  QUERY_OP_NOT);
	PUSH("cat",      QUERY_KW_TERM, QUERY_OP_OR);

	query_digest_txt(&qry, "Dog eats cat");

	query_print(qry, stderr);
	printf("\n");

	GET(0);
	GET(1);
	GET(3);
	GET(7);
	GET(8);
	GET(9);
	GET(10);
	GET(100);
	printf("\n");

	query_delete(qry);

	mhook_print_unfree();
	return 0;
}
