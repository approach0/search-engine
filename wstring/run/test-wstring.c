#include <stdio.h>
#include <string.h>
#include "wstring.h"

int main()
{
	int i;
	wchar_t *w = mbstr2wstr("牛顿apple万有引力");
	for (i = 0; i < wstr_len(w); i++) {
		printf("%C\n", w[i]);
	}

	char* s = wstr2mbstr(w);
	printf("%s\n", s);
	printf("multi-bytes mbstr_chars() = %lu\n", mbstr_chars(s));
	printf("multi-bytes strlen() = %lu\n", strlen(s));

	return 0;
}
