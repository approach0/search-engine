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
	void *p = __real_malloc(c);

	if (p)
		unfree++;
	return p;
}

void *__real_free(size_t);

void *__wrap_free(size_t c)
{
	unfree--;
	return __real_free(c);
}

void *__real_calloc(size_t, size_t);

void *__wrap_calloc(size_t nmemb, size_t size)
{
	void *p = __real_calloc(nmemb, size);

	if (p)
		unfree++;
	return p;
}
