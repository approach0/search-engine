#include "mhook/mhook.h"
#include "mathml-parser.h"

int main()
{
	struct optr_node *root;
	const char tex[] = "\\\\frac a b + \\\\root 3 \\\\of x";
	
	latexml_gen_mathml_file("math.xml.tmp", tex);
	printf("%s\n", tex);
	root = mathml_parse_file("math.xml.tmp");

	optr_print(root, stdout);
	optr_release(root);

	mhook_print_unfree();
	return 0;
}
