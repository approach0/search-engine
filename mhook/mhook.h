#pragma once
#include <stdint.h>
#include <malloc.h>

uint64_t mhook_unfree();
void mhook_print_unfree();

void *__wrap_malloc(size_t);
void *__wrap_free(size_t);
void *__wrap_calloc(size_t, size_t);
