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

static char *trans_symbol_(enum symbol_id id, int show_font)
{
	static char ret[TRANS_BUF_LEN];

	if (id >= SYMBOL_ID_a2Z_BEGIN) {
		if (id >= SYMBOL_ID_NUM_BEGIN /* small number */) {
			sprintf(ret, "`%u'", id - SYMBOL_ID_NUM_BEGIN);
			return ret;
		} else /* a-Z in different fonts */ {
			int r_id = id - SYMBOL_ID_a2Z_BEGIN; /* relative ID */
			int l_id = r_id % (2 * 26); /* letter ID (with font) */

			/* font */
			int font = r_id / (2 * 26);
			char font_str[128] = "";
			if (show_font)
				strcpy(font_str, math_font_name(font));

			if (l_id >= 26 /* uppercase */) {
				int o = l_id - 26; /* offset */
				sprintf(ret, "%s`%c'", font_str, 'A' + o);
			} else /* lowercase */ {
				int o = l_id; /* offset */
				sprintf(ret, "%s`%c'", font_str, 'a' + o);
			}
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


char *trans_symbol(enum symbol_id id)
{
	return trans_symbol_(id, true);
}

char *trans_symbol_wo_font(enum symbol_id id)
{
	return trans_symbol_(id, false);
}
