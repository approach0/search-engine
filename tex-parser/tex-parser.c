#include "head.h"

extern struct optr_node *grammar_optr_root;
extern bool grammar_err_flag;
extern char grammar_last_err_str[];
extern int  lexer_warning_flag;

static char *mk_scan_buf(const char *str, size_t *out_sz)
{
	char *buf;
	*out_sz = strlen(str) + 3;
	buf = malloc(*out_sz);
	sprintf(buf, "%s\n_", str);
	buf[*out_sz - 2] = '\0';

	return buf;
}

struct tex_parse_ret
tex_parse(const char *tex_str, size_t len, bool keep_optr)
{
	struct tex_parse_ret ret;
	YY_BUFFER_STATE state_buf;
	char *scan_buf;
	size_t scan_buf_sz;

	scan_buf = mk_scan_buf(tex_str, &scan_buf_sz);
	state_buf = yy_scan_buffer(scan_buf, scan_buf_sz);

	/* clear flags */
	grammar_err_flag = 0;
	lexer_warning_flag = 0;

	yyparse();

	yy_delete_buffer(state_buf);
	free(scan_buf);

	if (grammar_err_flag) {
		ret.code = PARSER_RETCODE_ERR;
		strcpy(ret.msg, grammar_last_err_str);
	} else {
		if (grammar_optr_root) {
			optr_assign_values(grammar_optr_root);
			ret.subpaths = optr_subpaths(grammar_optr_root);
			if (keep_optr) {
				ret.operator_tree = grammar_optr_root;
			} else {
				optr_release(grammar_optr_root);
				ret.operator_tree = NULL;
			}

			if (lexer_warning_flag) {
				ret.code = PARSER_RETCODE_WARN;
				strcpy(ret.msg, "character(s) escaped.");
			} else {
				ret.code = PARSER_RETCODE_SUCC;
				strcpy(ret.msg, "no error.");
			}
		} else {
			ret.code = PARSER_RETCODE_ERR;
			strcpy(ret.msg, "operator tree not generated.");
		}
	}

	return ret;
}
