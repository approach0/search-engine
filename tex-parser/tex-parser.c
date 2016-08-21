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

	/* create parser buffer */
	scan_buf = mk_scan_buf(tex_str, &scan_buf_sz);
	state_buf = yy_scan_buffer(scan_buf, scan_buf_sz);

	/* clear flags */
	grammar_err_flag = 0;
	lexer_warning_flag = 0;

	/* do parse */
	yyparse();

	/* free parser buffer */
	yy_delete_buffer(state_buf);
	free(scan_buf);

	/* return operator tree or not, depends on `keep_optr' */
	if (keep_optr)
		ret.operator_tree = grammar_optr_root;
	else
		ret.operator_tree = NULL;

	/* is there any grammar error? */
	if (grammar_err_flag) {
		/* grammar error */
		ret.code = PARSER_RETCODE_ERR;
		strcpy(ret.msg, grammar_last_err_str);
	} else {
		if (grammar_optr_root) {
			uint32_t max_path_id;

			optr_prune_nil_nodes(grammar_optr_root);
			max_path_id = optr_assign_values(grammar_optr_root);

			/*
			 * Inside optr_subpaths(), it uses a bitmap data
			 * structure, we need to make sure path_id is in
			 * a legal range.
			 */
			if (max_path_id <= MAX_SUBPATH_ID) {
				ret.subpaths = optr_subpaths(grammar_optr_root);

				if (lexer_warning_flag) {
					ret.code = PARSER_RETCODE_WARN;
					strcpy(ret.msg, "character(s) escaped.");
				} else {
					ret.code = PARSER_RETCODE_SUCC;
					strcpy(ret.msg, "no error.");
				}

			} else {
				LIST_CONS(ret.subpaths.li);
				ret.subpaths.n_lr_paths = 0;
				ret.subpaths.n_subpaths = 0;

				ret.code = PARSER_RETCODE_ERR;
				strcpy(ret.msg, "too many subpaths.");
			}

			if (!keep_optr)
				optr_release(grammar_optr_root);
		} else {
			ret.code = PARSER_RETCODE_ERR;
			strcpy(ret.msg, "operator tree not generated.");
		}
	}

	return ret;
}
