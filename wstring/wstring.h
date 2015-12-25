#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include "../config/config.h"

size_t mbstr_len(wchar_t *multibyte_string);
wchar_t *mbstr_copy(wchar_t *dest, const wchar_t *src);
wchar_t *mbstr2wstr(const char *multibyte_string);
