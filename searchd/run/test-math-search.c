#include "math-index/math-index.h"
#include "math-index/subpath-set.h"
#include "math-search.h"

int main()
{
	const char test_tex[] = "a + b + a/b + 1 = 2a";
	struct tex_parse_ret parse_ret;

	parse_ret = tex_parse(test_tex, 0, false);

	if (parse_ret.code == PARSER_RETCODE_SUCC) {
		printf("before math_search_prepare_qry():\n");
		subpaths_print(&parse_ret.subpaths, stdout);
		math_search_prepare_qry(&parse_ret.subpaths);

		printf("after math_search_prepare_qry():\n");
		subpaths_print(&parse_ret.subpaths, stdout);

		subpaths_release(&parse_ret.subpaths);
	} else {
		printf("parser error: %s\n", parse_ret.msg);
	}

	return 0;
}
