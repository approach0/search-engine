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
	static wchar_t w[MAX_WSTR_LEN];

	/* mbstowcs() will convert a string from the 
	 * current locale's multibyte encoding into a 
	 * wide character string. Wide character strings 
	 * are not necessarily unicode, but on Linux they 
	 * are. 
	 */
	setlocale(LC_ALL, "en_US.UTF-8");
	mbstowcs(w, multibyte_string, MAX_WSTR_LEN - 1);
	return w;
}
