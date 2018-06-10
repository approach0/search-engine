#include <stdio.h>
#include "hello/hello.h" /* another module */
#include "hello2.h" /* this module */

int main()
{
	/* print hello world twice */
	print_hello();
	print_world();
	print_hello_world();

	return 0;
}
