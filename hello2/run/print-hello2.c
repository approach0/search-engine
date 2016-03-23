#include <stdio.h>
#include "hello/hello.h" /* another module */
#include "hello2.h" /* own module */

int main()
{
	/* print hello world twice */
	print_hello();
	print_world();
	print_hello_world();

	return 0;
}
