#include "head.h"

#define TRANS_BUF_LEN 1024

char *trans_token(enum token_id id)
{
	static char ret[TRANS_BUF_LEN];

	/* degree tokens */
	if (id > T_NIL) {
		if (id < T_DEGREE_VALVE) {
			sprintf(ret, "%dsons", id);
			return ret;
		} else if (id == T_DEGREE_VALVE) {
			sprintf(ret, "ge%dsons", T_DEGREE_VALVE);
			return ret;
		} else if (id <= T_MAX_RANK) {
			sprintf(ret, "rank%d", id - T_DEGREE_VALVE);
			return ret;
		}
	}

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
		sprintf(ret, "unlisted");
	}

	return ret;
}

char *trans_symbol(enum symbol_id id)
{
	static char ret[TRANS_BUF_LEN];

	if (id >= S_N) {
		/* number symbols */
		if (id < S_N + 26 /* is lowercase alphabet */) {
			sprintf(ret, "`%c'", id - S_N + 'a');
			return ret;
		} else if (id < S_N + 52 /* is uppercase alphabet */) {
			sprintf(ret, "`%c'", id - S_N - 26 + 'A');
			return ret;
		} else /* is small number */ {
			sprintf(ret, "`%u'", id - S_N - 52);
			return ret;
		}
	}

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
		sprintf(ret, "unlisted");
	}

	return ret;
}
