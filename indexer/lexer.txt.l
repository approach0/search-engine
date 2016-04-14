%option prefix="txt"
%option noyywrap

%{
#include <stdio.h>
#include "txt-seg.h"

int test(char*);
%}

engword [a-zA-Z]
chinese [\x80-\xf3]

%%
\n                                     {}
{engword}+                             { printf("English word:`%s'\n", txttext); }
{chinese}+                             { test(txttext); }
.                                      {}
%%
static LIST_IT_CALLBK(print)
{
	LIST_OBJ(struct term_list_node, p, ln);

	printf("%S", p->term);
	if (pa_now->now != pa_head->last)
		printf(", ");

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(list_release, struct term_list_node,
                  ln, free(p));

int test(char *txt)
{
	list li = LIST_NULL;

	li = text_segment(txt);
	list_foreach(&li, &print, NULL);
	printf("\n");
	list_release(&li);

	return 0;
}
