#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "strmap.h"

int main()
{
	char test_str[] = "world";
	struct strmap *m = strmap_new();
	void **val = strmap_val_ptr(m, "hello");
	*val = &test_str;
	strmap_free(m);

	mhook_print_unfree();
	return 0;
}
