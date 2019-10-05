#include <stdio.h>
#include "mhook/mhook.h"
#include "common/common.h"
#include "math-index.h"

int main()
{
	math_index_t index = math_index_load("./tmp", 3 MB);

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	printf("load finished.\n");

	math_index_print(index);

	printf("closing ...\n");
	math_index_close(index);

	mhook_print_unfree();
	return 0;
}
