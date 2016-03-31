%{
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "gen-symbol.h"
#include "gen-token.h"
#include "yy.h"
#include "optr.h"
#include "config.h"

/* global interfaces */
struct optr_node *grammar_optr_root = NULL;
bool grammar_err_flag = 0;
char grammar_last_err_str[MAX_GRAMMAR_ERR_STR_LEN] = "";

#define OPTR_ATTACH(_ret, _child1, _child2, _father) \
	optr_attach(_child1, _father); \
	optr_attach(_child2, _father); \
	_ret = grammar_optr_root = _father;
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
%token <nd> TIMES
%token <nd> DIV
%token <nd> FRAC
%token <nd> FRAC__
%token <nd> ABOVE
%token _OVER
%token <nd> SUM_CLASS
%token <nd> SEP_CLASS
%token <nd> REL_CLASS
%token <nd> FUN_CLASS
%token <nd> PRIME
%token <nd> SUBSCRIPT
%token <nd> SUPSCRIPT

%token _L_BRACKET
%token _L_DOT
%token _L_ANGLE
%token _L_SLASH
%token _L_HAIR
%token _L_BSLASH
%token _L_ARROW
%token _L_TEX_BRACE
%token _L_TEX_BRACKET
%token _R_BRACKET
%token _R_DOT
%token _R_ANGLE
%token _R_SLASH
%token _R_HAIR
%token _R_ARROW
%token _R_TEX_BRACE
%token _R_TEX_BRACKET

%type <nd> tex
%type <nd> term
%type <nd> factor
%type <nd> pack
%type <nd> atom
%type <nd> script
%type <nd> s_atom
%type <nd> pair

/* associativity rules (order-sensitive)  */
%right _OVER

%right SEP_CLASS
%right REL_CLASS

%right ABOVE

%left NULL_REDUCE
%left ADD NEG
%nonassoc PRIME
%right    SUPSCRIPT SUBSCRIPT
%left TIMES DIV
%nonassoc _L_BRACKET     _R_BRACKET
%nonassoc _L_DOT         _R_DOT
%nonassoc _L_ANGLE       _R_ANGLE
%nonassoc _L_SLASH       _R_SLASH
%nonassoc _L_HAIR        _R_HAIR
%nonassoc _L_ARROW       _R_ARROW
%nonassoc _L_TEX_BRACE   _R_TEX_BRACE
%nonassoc _L_TEX_BRACKET _R_TEX_BRACKET
%nonassoc _L_CEIL        _R_CEIL
%nonassoc _L_FLOOR       _R_FLOOR

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
}
| tex ADD {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| tex NEG term {
	struct optr_node *neg;
	OPTR_ATTACH(neg, $3, NULL, $2);
	OPTR_ATTACH($$, neg, NULL, $1);
}
| tex NEG {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| tex REL_CLASS tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| tex SEP_CLASS tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| tex _OVER tex {
	struct optr_node *frac;
	frac = optr_alloc(S_frac, T_FRAC, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $1, $3, frac);
}
| tex ABOVE tex {
	/* example: {a \above 2pt c} */
	OPTR_ATTACH($$, $1, $3, $2);
}
;

term: factor {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| term factor {
	struct optr_node *times;
	times = optr_alloc(S_times, T_TIMES, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $1, $2, times);
}
| term TIMES factor {
	OPTR_ATTACH($$, $1, $3, $2);
}
| term DIV factor {
	OPTR_ATTACH($$, $1, $3, $2);
}
;

factor: pack {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| factor PRIME {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| factor script {
	struct optr_node *base;
	base = optr_alloc(S_base, T_BASE, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $1, NULL, base);
	OPTR_ATTACH($$, base, NULL, $2);
}
;

pack: atom {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| pair {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| _L_TEX_BRACE tex _R_TEX_BRACE {
	/* tex braces is invisible in math rendering */
	OPTR_ATTACH($$, NULL, NULL, $2);
}
;

atom: VAR {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| NUM {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| FUN_CLASS {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| SUM_CLASS {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| FRAC__ {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| FRAC atom atom {
	OPTR_ATTACH($$, $2, $3, $1);
}
;

s_atom: atom {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| _L_TEX_BRACE tex _R_TEX_BRACE {
	/* tex braces is invisible in math rendering */
	OPTR_ATTACH($$, NULL, NULL, $2);
}
| ADD {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| NEG {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| TIMES {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
;

script: SUBSCRIPT s_atom {
	struct optr_node *hanger;
	hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, $1);
	OPTR_ATTACH($$, $1, NULL, hanger);
}
| SUPSCRIPT s_atom {
	struct optr_node *hanger;
	hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, $1);
	OPTR_ATTACH($$, $1, NULL, hanger);
}
| SUBSCRIPT s_atom SUPSCRIPT s_atom {
	struct optr_node *hanger;
	hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, $1);
	OPTR_ATTACH($$, $4, NULL, $3);
	OPTR_ATTACH($$, $1, $3, hanger);
}
| SUPSCRIPT s_atom SUBSCRIPT s_atom {
	struct optr_node *hanger;
	hanger = optr_alloc(S_hanger, T_HANGER, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, $1);
	OPTR_ATTACH($$, $4, NULL, $3);
	OPTR_ATTACH($$, $1, $3, hanger);
}
;

pair: _L_BRACKET tex _R_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_bracket, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
/* bracket array */
| _L_DOT tex _R_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_array, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_BRACKET tex _R_DOT {
	struct optr_node *pair;
	pair = optr_alloc(S_array, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
/* range */
| _L_BRACKET tex _R_TEX_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_bracket, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_TEX_BRACKET tex _R_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_bracket, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
/* others */
| _L_ANGLE tex _R_ANGLE {
	struct optr_node *pair;
	pair = optr_alloc(S_angle, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_SLASH tex _R_SLASH {
	struct optr_node *pair;
	pair = optr_alloc(S_slash, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_HAIR tex _R_HAIR {
	struct optr_node *pair;
	pair = optr_alloc(S_hair, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_ARROW tex _R_ARROW {
	struct optr_node *pair;
	pair = optr_alloc(S_arrow, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_TEX_BRACKET tex _R_TEX_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_bracket, T_GROUP, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_CEIL tex _R_CEIL {
	struct optr_node *pair;
	pair = optr_alloc(S_ceil, T_CEIL, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_FLOOR tex _R_FLOOR {
	struct optr_node *pair;
	pair = optr_alloc(S_floor, T_FLOOR, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
;
%%

int yyerror(const char *msg)
{
	strcpy(grammar_last_err_str, msg);

	/* set root to NULL to avoid double free */
	grammar_optr_root = NULL;
	grammar_err_flag = 1;

	return 0;
}
