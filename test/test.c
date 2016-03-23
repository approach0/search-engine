#include <stdio.h>
#include <stdlib.h>
#include "txt-seg/txt-seg.h"

static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct term_list_node, p, ln);
	wchar_t *term = p->term;

	printf("%S", p->term);
	if (pa_now->now == pa_head->last)
		printf(".");
	else
		printf(", ");

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(list_release, struct term_list_node,
                  ln, free(p));

void test()
{
	char txt[] = "这是一个100分之99标准的a测试";
	list li = LIST_NULL;
	text_segment_init("../jieba/clone/dict");
	li = text_segment(txt);
	text_segment_free();

	list_foreach(&li, &print, NULL);
	printf("\n");

	list_release(&li);
}
