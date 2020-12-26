#include <stdio.h>
#include "mhook/mhook.h"
#include "tex-parser/head.h"
#include "math-index-v3/subpath-set.h"

int main()
{
	//char test_tex[] = "abcdefabcd\\qvar{x} + \\mathfrak{Y}";
	//char test_tex[] = "a^2+b^2=c^2";
	char test_tex[] = "\\frac{a}{b+c} + \\frac{b}{a+c} + \\frac{c}{a+b} = 4";

	enum subpath_set_opt opt = SUBPATH_SET_FOR_INDEX;
	//enum subpath_set_opt opt = SUBPATH_SET_FOR_QUERY;

	struct tex_parse_ret parse_ret = tex_parse(test_tex);
	if (parse_ret.code != PARSER_RETCODE_ERR) {
		/* print operator tree */
		optr_print(parse_ret.operator_tree, stdout);
		optr_release(parse_ret.operator_tree);

		/* print leaf-root paths */
		printf("\n === Leaf-Root Paths === \n");
		subpaths_print(&parse_ret.lrpaths, stdout);

		/* generate and print subpath set */
		linkli_t set = subpath_set(parse_ret.lrpaths, opt);
		printf("\n === Subpath Set === \n");
		print_subpath_set(set);

		li_free(set, struct subpath_ele, ln, free(e));
		subpaths_release(&parse_ret.lrpaths);
	} else {
		fprintf(stderr, "Parsing error: %s\n", parse_ret.msg);
	}

	mhook_print_unfree();
	return 0;
}
