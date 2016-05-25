#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "list/list.h"
#include "snippet.h"

#define ADD( _pos, _term) \
	snippet_add_pos(&div_list, _term, _pos, strlen(_term));

int main(void)
{
	list div_list = LIST_NULL;
	FILE *fh = fopen("text.tmp", "r");

	if (fh == NULL) {
		printf("cannot open file.\n");
		return 1;
	}

	ADD(78, "Startup");
	ADD(183, "Combinator");
	ADD(86, "News");
	//ADD(0, "Hacker");
	ADD(495, "and");
	ADD(181, "Y");
	ADD(7, "News");

	snippet_read_file(fh, &div_list);
	snippet_hi_print(&div_list);

	snippet_free_div_list(&div_list);
	fclose(fh);
	return 0;
}
