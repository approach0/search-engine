%{
#include "enum-symbol.h"
#include "enum-token.h"
#include <stdio.h>

extern int yyparse();
extern int yyerror(const char *);
extern int yylex();
%}

%union {
	int t;
}

%error-verbose

%token <t> NUM 

%type <t> tex

%%
doc: tex '\n';

tex: 
NUM {
	printf("a+b\n");
}
|tex '+' NUM {
	printf("a+b\n");
};
%%

int yyerror(const char *msg)
{
	printf("err: %s\n", msg);
}
