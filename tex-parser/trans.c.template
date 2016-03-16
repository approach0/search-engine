#include "enum-token.h"
#include "enum-symbol.h"
#include <stdio.h>

#define TRANS_BUF_LEN 1024

char *trans_token(enum token_id id)
{
	static char ret[TRANS_BUF_LEN];

	switch (id) {
	/* example:
	case T_NIL:
		sprintf(ret, "NIL");
		break;
	case T_VAR:
		sprintf(ret, "VAR");
		break;
	*/

	/* T_INSERT_HERE */

	default:
		sprintf(ret, "unknown");
	}

	return ret;
}

char *trans_symbol(enum symbol_id id)
{
	static char ret[TRANS_BUF_LEN];

	switch (id) {
	/* example:
	case S_NIL:
		sprintf(ret, "nil");
		break;
	case S_alpha:
		sprintf(ret, "alpha");
		break;
	case S_BIGNUM:
		sprintf(ret, "bignum");
		break;
	*/

	/* S_INSERT_HERE */

	default:
		if (id < S_N) {
			sprintf(ret, "unlisted");
		} else if (id < S_N + 26 /* is lowercase alphabet */) {
			sprintf(ret, "`%c'", id - S_N + 'a');
		} else if (id < S_N + 52 /* is uppercase alphabet */) {
			sprintf(ret, "`%c'", id - S_N - 26 + 'A');
		} else /* is small number */ {
			sprintf(ret, "`%u'", id - S_N - 52);
		}
	}

	return ret;
}
