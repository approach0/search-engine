#include <stdio.h>
#include <stdlib.h>
#include "txt-seg.h"

static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct term_list_node, p, ln);
	wchar_t *term = p->term;
	printf("%S, ", p->term);
	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(list_release, struct term_list_node, 
                  ln, free(p));

int main()
{
	char txt[] = "我爱北京天安门";
	list li;

	text_segment_init();
	li = text_segment(txt);
	text_segment_free();

	list_foreach(&li, &print, NULL);
	printf("\n");

	list_release(&li);

	return 0;
}
