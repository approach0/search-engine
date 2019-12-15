#include "mhook/mhook.h"
#include "head.h"

void test(const char *tex)
{
	struct tex_parse_ret ret = tex_parse(tex, 0, true, false);
	printf("testing `%s'\n", tex);

	if (ret.code != PARSER_RETCODE_ERR) {
		optr_print(ret.operator_tree, stdout);
		optr_release(ret.operator_tree);
		subpaths_print(&ret.subpaths, stdout);
		subpaths_release(&ret.subpaths);
	}

	mhook_print_unfree();
	printf("\n");
}

int main()
{
	test("3.14");
	test("a");
	test("1");
	test("0");
	test("2");

	test("a_b");
	test("_a");
	test("a_2'");
	test("a'_2");

	test("a+b+c");

	test("a_1^2");

	test("\\sum_i a_i");
	test("\\sum a_i");

	test("a-b+c");
	test("a+b-c");
	test("a+b-c+");
	test("a+b-c-");
	test("-bc");

	return 0;
}
