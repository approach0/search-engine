#include <assert.h>
#include "head.h"

#ifdef TEX_PARSER_USE_LATEXML
#include "mathml-parser.h"
#endif

extern struct optr_node *grammar_optr_root;
extern bool grammar_err_flag;
extern char grammar_last_err_str[];
extern int  lexer_warning_flag;

static void strip_newlines(char *str, size_t len)
{
	for (int i = 0; i < len; i++) {
		if (str[i] == '\n')
			str[i] = ' ';
	}
}

/*
 * mk_scan_buf() appends '\n' and '\0' after str such that:
 * (1) make a _EOL so that grammar can terminate,
 * (2) yy_scan_buffer() requires double-null string (YY_END_OF_BUFFER_CHAR).
 */
static char *mk_scan_buf(const char *str, size_t *out_sz)
{
	char *buf;
	size_t len = strlen(str);
	*out_sz = len + 3;
	buf = malloc(*out_sz);
	sprintf(buf, "%s\n_", str);
	buf[*out_sz - 2] = '\0';

	/* to avoid multiple parsing, we should strip '\n' from original string */
	strip_newlines(buf, len);
	return buf;
}

struct tex_parse_ret
tex_parse(const char *tex_str, size_t len, bool keep_optr, bool lr_path)
{
	struct tex_parse_ret ret;
#ifndef TEX_PARSER_ALWAYS_LATEXML
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

	/* avoid memory leakage */
	yylex_destroy();
#else
	grammar_err_flag = 1;
#endif

#ifdef TEX_PARSER_USE_LATEXML
	if (grammar_err_flag) {
		struct optr_node *mathml_root;
		latexml_gen_mathml_file("math.xml.tmp", tex_str);
		mathml_root = mathml_parse_file("math.xml.tmp");
		// optr_print(mathml_root, stdout);
		assert(NULL == grammar_optr_root);

		grammar_optr_root = mathml_root;
		grammar_err_flag = 0;
	}
#endif

	/* is there any grammar error? */
	if (grammar_err_flag) {
		/* grammar error */
		ret.code = PARSER_RETCODE_ERR;
		strcpy(ret.msg, grammar_last_err_str);
	} else {
		if (grammar_optr_root) {
			uint32_t max_path_id;

			optr_prune_nil_nodes(grammar_optr_root);

			if (is_single_node(grammar_optr_root)) {
				/* HACKY: In order to process this single-node tree correctly
				 * in our model, add an 'isolated-node' on top of it. */
				struct optr_node *iso = optr_alloc(S_NIL, T_NIL,
				                                   WC_COMMUT_OPERATOR);
				grammar_optr_root = optr_attach(grammar_optr_root, iso);
			}

			max_path_id = optr_assign_values(grammar_optr_root);

			/*
			 * Inside optr_subpaths(), it uses a bitmap data
			 * structure, we need to make sure path_id is in
			 * a legal range.
			 */
			ret.subpaths = optr_subpaths(grammar_optr_root, lr_path);

#if 0       /* print */
			printf("tex: `%s', max_path_id: %u.\n", tex_str, max_path_id);
			optr_print(grammar_optr_root, stdout);
			subpaths_print(&ret.subpaths, stdout);
#endif
			if (max_path_id > MAX_SUBPATH_ID) {
				ret.code = PARSER_RETCODE_WARN;
				sprintf(ret.msg, "too many subpaths (%u/%u).",
				        max_path_id, MAX_SUBPATH_ID);
			} else if (lexer_warning_flag) {
				ret.code = PARSER_RETCODE_WARN;
				strcpy(ret.msg, "character(s) escaped.");
			} else {
				ret.code = PARSER_RETCODE_SUCC;
				sprintf(ret.msg, "no error (max path ID = %u).",
					max_path_id);
			}

			if (!keep_optr)
				optr_release(grammar_optr_root);
		} else {
			ret.code = PARSER_RETCODE_ERR;
			strcpy(ret.msg, "operator tree not generated.");
		}
	}

	/* return operator tree or not, depends on `keep_optr' */
	if (keep_optr)
		ret.operator_tree = grammar_optr_root;
	else
		ret.operator_tree = NULL;

	return ret;
}
