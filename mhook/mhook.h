#pragma once
#include <stdint.h>
#include <malloc.h>

uint64_t mhook_unfree();
uint64_t mhook_tot_allocs();

void mhook_print_unfree();

void *__wrap_malloc(size_t);
void *__wrap_calloc(size_t, size_t);
void __wrap_free(void*);
