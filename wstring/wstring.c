#include <wstring.h>

size_t mbstr_len(wchar_t *multibyte_string)
{
	return wcslen(multibyte_string);
}

wchar_t *mbstr_copy(wchar_t *dest, const wchar_t *src)
{
	wcscpy(dest, src);
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
