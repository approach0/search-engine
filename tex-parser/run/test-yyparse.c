#include "mhook/mhook.h"
#include "head.h"

int main()
{
	struct tex_parse_ret ret = tex_parse("a+b", 0, true, false);

	optr_print(ret.operator_tree, stdout);
	optr_release(ret.operator_tree);

	if (ret.code != PARSER_RETCODE_ERR) {
		subpaths_print(&ret.subpaths, stdout);
		subpaths_release(&ret.subpaths);
	}

	mhook_print_unfree();
	return 0;
}
