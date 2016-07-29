#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "list/list.h"
#include "snippet.h"

#define ADD( _pos, _term) \
	snippet_push_highlight(&hi_li, _term, _pos, strlen(_term));

int main(void)
{
	list hi_li = LIST_NULL;
	FILE *fh = fopen("test-snippet/sample.txt", "r");

	if (fh == NULL) {
		printf("cannot open file.\n");
		return 1;
	}

	ADD(363, "hello");
	ADD(3908, "world");
	ADD(4276, "world");
	ADD(4313, "world");

	snippet_read_file(fh, &hi_li);
	snippet_hi_print(&hi_li);

	snippet_free_highlight_list(&hi_li);
	fclose(fh);
	return 0;
}
