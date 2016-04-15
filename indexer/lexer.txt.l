%option prefix="txt"
%option noyywrap

%{
#include <stdio.h>

int handle_chinese(char*);
int handle_english(char*);
%}

engword [a-zA-Z]
chinese [\x80-\xf3]

%%
{engword}+                             { handle_english(txttext); }
{chinese}+                             { handle_chinese(txttext); }
"[imath]".*"[/imath]"                  { printf("inline math: `%s'\n", txttext); }
\n                                     {}
.                                      {}
%%
