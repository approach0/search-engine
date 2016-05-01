#include <stdlib.h> /* for free() */
#include <string.h> /* for memcpy() */

#include "term-offset-conv.h"
#include "wstring/wstring.h"

static wchar_t *wstr_txt = NULL;

size_t term_offset_conv_init(char *txt)
{
	wchar_t *tmp = mbstr2wstr(txt);
	size_t   n_chars = mbstr_chars(txt);
	size_t   n_bytes = sizeof(wchar_t) * (n_chars + 1);

	wstr_txt = malloc(n_bytes);
	memcpy(wstr_txt, tmp, n_bytes);

	return n_chars;
}

void term_offset_conv_uninit(void)
{
	if (wstr_txt)
		free(wstr_txt);
}

uint32_t term_bytes_offset(uint32_t term_chars_offset)
{
	wchar_t save;
	uint32_t ret;

	save = wstr_txt[term_chars_offset];
	wstr_txt[term_chars_offset] = L'\0';
	ret = (uint32_t)mbstr_bytes(wstr_txt);
	wstr_txt[term_chars_offset] = save;

	return ret;
}
