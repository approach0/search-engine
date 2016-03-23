#pragma once
#include <stddef.h>
#include <wchar.h>

size_t   mbstr_len(wchar_t *multibyte_string);
wchar_t *mbstr_copy(wchar_t *dest, const wchar_t *src);
wchar_t *mbstr2wstr(const char *multibyte_string);
char    *wstr2mbstr(const wchar_t *wide_string);
