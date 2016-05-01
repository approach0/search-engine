#include <stdlib.h>
#include <locale.h>
#include <string.h>
#include "wstring.h"
#include "config.h"

size_t wstr_len(wchar_t *wstr)
{
	return wcslen(wstr);
}

wchar_t *wstr_copy(wchar_t *dest, const wchar_t *src)
{
	wcscpy(dest, src);
	return dest;
}

size_t mbstr_chars(const char *mbstr)
{
	return mbstowcs(NULL, mbstr, 0);
}

wchar_t *mbstr2wstr(const char *multibyte_string)
{
	static wchar_t retstr[MAX_WSTR_CONV_BUF_LEN];

	/* mbstowcs() will convert a string from the 
	 * current locale's multibyte encoding into a 
	 * wide character string. Wide character strings 
	 * are not necessarily unicode, but on Linux they 
	 * are.
	 */
	setlocale(LC_ALL, "en_US.UTF-8");
	mbstowcs(retstr, multibyte_string, MAX_WSTR_CONV_BUF_LEN);
	return retstr;
}

char *wstr2mbstr(const wchar_t *wide_string)
{
	static char retstr[MAX_STR_CONV_BUF_LEN];

	setlocale(LC_ALL, "en_US.UTF-8");
	wcstombs(retstr, wide_string, MAX_STR_CONV_BUF_LEN);

	return retstr;
}

size_t mbstr_bytes(const wchar_t *wstr)
{
	return wcstombs(NULL, wstr, 0);
}
