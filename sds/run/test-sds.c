#include <stdio.h>
#include "mhook/mhook.h"
#include "sds.h"

int main()
{
	sds s = sdsnew("Hello");
	s = sdscat(s, " ");
	s = sdscat(s, "World");
	printf("%s\n", s);

	int cnt;
	sds *tokens = sdssplitlen(s, sdslen(s), " ", 1, &cnt);
	for (int i = 0; i < cnt; i++) {
		printf("%s\n", tokens[i]);
	}

	for (int i = 0; i < cnt; i++)
		sdsfree(tokens[i]);
	free(tokens);
	sdsfree(s);

	mhook_print_unfree();
	return 0;
}
