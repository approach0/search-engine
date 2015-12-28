%{
#include <stdio.h>
#include "enum-symbol.h"
#include "enum-token.h"
#include "yy.h"
%}

%union {
	char t[128];
}

%error-verbose
%start doc /* start rule */

%token <t> NUM 

%type <t> tex

%%
doc: line | doc line; 

line: tex '\n';

tex:
{
	printf("null\n");
}
| NUM {
	printf("%s\n", $1);
}
|tex '+' NUM {
	printf("+%s\n", $3);
};
%%

int yyerror(const char *msg)
{
	printf("err: %s\n", msg);
}
