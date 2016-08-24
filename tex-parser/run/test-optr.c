#include "mhook/mhook.h"
#include "head.h"

int main()
{
	struct optr_node *root = optr_alloc(S_plus, T_ADD, 0);
	struct optr_node *a = optr_alloc(S_alpha, T_VAR, 0);
	struct optr_node *b = optr_alloc(S_phi, T_VAR, 0);
	optr_attach(a, root);
	optr_attach(b, root);

	optr_print(root, stdout);
	optr_release(root);

	mhook_print_unfree();
	return 0;
}
