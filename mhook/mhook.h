#pragma once
#include <stdint.h>
#include <malloc.h>
#include <string.h>

int64_t mhook_unfree();
int64_t mhook_tot_allocs();

void mhook_print_unfree();

void *__wrap_malloc(size_t);
void *__wrap_calloc(size_t, size_t);
void *__wrap_realloc(void*, size_t);
void *__wrap_strdup(const char*);
void __wrap_free(void*);
