#include <stdio.h>
#include "wstring.h"

int main()
{
	int i;
	wchar_t *w = mbstr2wstr("牛顿apple万有引力");
	for (i = 0; i < mbstr_len(w); i++) {
		printf("%C\n", w[i]);
	}

	char* s = wstr2mbstr(w);
	printf("%s\n", s);

	return 0;
}
