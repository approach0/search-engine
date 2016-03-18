#include <stdio.h>
#include "hello.h"
#include "hello2.h"

int main()
{
	/* print hello world twice */
	print_hello();
	print_world();
	printf("\n");
	print_hello_world();
	printf("\n");

	return 0;
}
