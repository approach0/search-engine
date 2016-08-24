#include <stdio.h>
#include "mhook.h"

static uint64_t unfree = 0;
static uint64_t tot_allocs = 0;

uint64_t mhook_unfree()
{
	return unfree;
}

uint64_t mhook_tot_allocs()
{
	return unfree;
}

void mhook_print_unfree()
{
	printf("Total memory allocs: %lu.\n", tot_allocs);
	printf("Unfree memory allocs: %lu.\n", unfree);
}

/* malloc() hook */
void *__real_malloc(size_t);

void *__wrap_malloc(size_t c)
{
	void *p = __real_malloc(c);

	if (p) {
		unfree++;
		tot_allocs++;
	}

	return p;
}

/* calloc() hook */
void *__real_calloc(size_t, size_t);

void *__wrap_calloc(size_t nmemb, size_t size)
{
	void *p = __real_calloc(nmemb, size);

	if (p) {
		unfree++;
		tot_allocs++;
	}

	return p;
}

/* free() hook */
void __real_free(void*);

void __wrap_free(void *p)
{
	if (p)
		unfree--;

	__real_free(p);
}
