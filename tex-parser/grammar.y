%{
#include <stdio.h>
#include "gen-symbol.h"
#include "gen-token.h"
#include "yy.h"
#include "optr.h"

struct optr_node *grammar_optr_root = NULL;

#define OPTR_ATTACH(_ret, _child1, _child2, _father) \
	struct optr_node *_n = _father; \
	optr_attach(_child1, _n); \
	optr_attach(_child2, _n); \
	_ret = grammar_optr_root = _n;
%}

%union {
	struct optr_node* nd;
}

%error-verbose
%start doc /* start rule */

%token <nd> NUM 
%token <nd> VAR
%token <nd> ADD 
%token <nd> NEG 
%token <nd> SUM_CLASS 
%token <nd> SEP_CLASS 
%token <nd> REL_CLASS 
%token <nd> FUN_CLASS 

%type <nd> tex
%type <nd> term 
%type <nd> factor 
%type <nd> pack 
%type <nd> atom 

%left NULL_REDUCE

%%
doc: line | doc line; 

line: tex '\n';

tex: %prec NULL_REDUCE {
	OPTR_ATTACH($$, NULL, NULL, optr_alloc(S_NIL, T_NIL, WC_NORMAL_LEAF));	
}
| term {
	OPTR_ATTACH($$, NULL, NULL, $1);	
}
| tex ADD term {
	OPTR_ATTACH($$, $1, $3, $2);	
};

term: factor {
	OPTR_ATTACH($$, NULL, NULL, $1);	
};

factor: pack {
	OPTR_ATTACH($$, NULL, NULL, $1);	
}

pack: atom {
	OPTR_ATTACH($$, NULL, NULL, $1);	
};

atom: VAR {
	OPTR_ATTACH($$, NULL, NULL, $1);	
}
| NUM {
	OPTR_ATTACH($$, NULL, NULL, $1);	
};
%%

int yyerror(const char *msg)
{
	printf("err: %s\n", msg);
}
