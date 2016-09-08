#pragma once
#include <stddef.h>
#include <wchar.h>

size_t   wstr_len(wchar_t *); /* the number of wide characters */
wchar_t *wstr_copy(wchar_t *dest, const wchar_t *src);

/* The number of characters in multibyte string: */
size_t mbstr_chars(const char*);

/* The number of bytes that make up the converted multibyte string: */
size_t mbstr_bytes(const wchar_t*);

wchar_t *mbstr2wstr(const char *multibyte_string);
char    *wstr2mbstr(const wchar_t *wide_string);

/* string to lower case */
#include <ctype.h>
static __inline void eng_to_lower_case(char *str, size_t n)
{
	size_t i;
	for(i = 0; i < n; i++)
		str[i] = tolower(str[i]);
}

#include <wctype.h>
static __inline void eng_to_lower_case_w(wchar_t *str, size_t n)
{
	size_t i;
	for(i = 0; i < n; i++)
		str[i] = towlower(str[i]);
}

/*
 * Many of C’s string functions are locale-independent
 * and they just look at zero-terminated byte sequences:
 * strcpy strncpy strcat strncat
 * strcmp strncmp strdup strchr strrchr
 * strcspn strspn strpbrk strstr strtok
 *
 * Some of these (e.g. strcpy) can equally be used for
 * single-byte (ISO 8859-1) and multi-byte (UTF-8)
 * encoded character sets, as they need no notion of
 * how many byte long a character is, while others
 * (e.g., strchr) depend on one character being encoded
 * in a single char value and are of less use for UTF-8
 * (strchr still works fine if you just search for an
 * ASCII character in a UTF-8 string).
 *
 * ref: https://www.cl.cam.ac.uk/~mgk25/unicode.html
 */
