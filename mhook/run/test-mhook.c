#include <stdio.h>
#include <stdlib.h>
#include "mhook.h"

int main()
{
	void *p;

	free(malloc(1));
	mhook_print_unfree();

	p = calloc(1, 2);
	mhook_print_unfree();

	p = realloc(p, 10);
	free(p);
	mhook_print_unfree();

	printf("realloc used as malloc()\n");
	p = realloc(NULL, 1);
	mhook_print_unfree();

	printf("realloc used as free()\n");
	p = realloc(p, 0);
	mhook_print_unfree();

	free(strdup("hello"));
	mhook_print_unfree();
	return 0;
}
