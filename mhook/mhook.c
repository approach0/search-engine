#include <stdio.h>
#include "mhook.h"

static uint64_t unfree = 0;

uint64_t mhook_unfree()
{
	return unfree;
}

void mhook_print_unfree()
{
	printf("Unfree mallocs: %lu.\n", unfree);
}

void *__real_malloc(size_t);

void *__wrap_malloc(size_t c)
{
	unfree++;
	return __real_malloc(c);
}

void *__real_free(size_t);

void *__wrap_free(size_t c)
{
	unfree--;
	return __real_free(c);
}
