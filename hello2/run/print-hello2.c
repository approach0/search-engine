#include <stdio.h>
#include "hello.h"

int main()
{
	int i;
	/* print hello world twice */
	for (i = 0; i < 2; i++) {
		print_hello();
		print_world();
	}
	return 0;
}
