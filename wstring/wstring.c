#include <stdio.h>
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
	static wchar_t retstr[MAX_WSTR_CONV_BUF_LEN + 1] = L"";

	/* mbstowcs() will convert a string from the 
	 * current locale's multibyte encoding into a 
	 * wide character string. Wide character strings 
	 * are not necessarily unicode, but on Linux they 
	 * are.
	 *
	 * Use `locale -a' to check available locales on your system.
	 */
	if (setlocale(LC_ALL, "C.UTF-8") == NULL) {
		fprintf(stderr, "No support for POSIX standards-compliant UTF-8 locale!!\n");
		return retstr;
	}

	/* at most MAX_WSTR_CONV_BUF_LEN wide characters are written to retstr (not including the L'\0') */
	size_t n_chars = mbstowcs(retstr, multibyte_string, MAX_WSTR_CONV_BUF_LEN);
	if (n_chars == -1)
		return retstr;

	retstr[n_chars] = L'\0';
	return retstr;
}

char *wstr2mbstr(const wchar_t *wide_string)
{
	static char retstr[MAX_STR_CONV_BUF_LEN + 1] = "";

	if (setlocale(LC_ALL, "C.UTF-8") == NULL) {
		fprintf(stderr, "No support for POSIX standards-compliant UTF-8 locale!!\n");
		return retstr;
	}

	/* at most MAX_STR_CONV_BUF_LEN bytes are written to retstr (not including the '\0') */
	size_t n_chars = wcstombs(retstr, wide_string, MAX_STR_CONV_BUF_LEN);
	if (n_chars == -1)
		return retstr;

	retstr[n_chars] = '\0';
	return retstr;
}

size_t mbstr_bytes(const wchar_t *wstr)
{
	return wcstombs(NULL, wstr, 0);
}
