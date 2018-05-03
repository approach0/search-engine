#include "mhook/mhook.h"
#include "head.h"
#include "mathml-parser.h"

int main()
{
	struct optr_node *root;

	const char tex[] = "\\root 3 \\of x";
	latexml_gen_mathml_file("math.xml.tmp", tex);

	root = mathml_parse_file("math.xml.tmp");

	optr_print(root, stdout);
	optr_release(root);

	mhook_print_unfree();
	return 0;
}
