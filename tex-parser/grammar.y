%{
#include "head.h"

/* parser interfaces */
struct optr_node *grammar_optr_root = NULL;
bool grammar_err_flag = 0;
char grammar_last_err_str[MAX_GRAMMAR_ERR_STR_LEN] = "";

/* handy macro */
#define OPTR_ATTACH(_ret, _child1, _child2, _father) \
	optr_attach(_child1, _father); \
	optr_attach(_child2, _father); \
	_ret = grammar_optr_root = _father;
%}

/* =========================
 * data type and destructor
 * ========================*/
%union {
	struct optr_node* nd;
}

%destructor {
	if ($$) {
		optr_release($$);
		$$ = NULL;
	}
} <nd>

%define parse.error verbose

/* ====================
 * token definitions
 * ===================*/
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
%token <nd> BINOM
%token <nd> BINOM__
%token <nd> CHOOSE
%token <nd> SQRT
%token <nd> ROOT
%token _OF
%token <nd> VECT
%token <nd> MODULAR
%token <nd> FACT
%token _QVAR
%token <nd> TAB_ROW TAB_COL
%token _BEGIN_MAT _END_MAT
%token _STACKREL
%token _BUILDREL
%token _SET_REL
%token _UNDER_OVER_BRACE
%token <nd> X_ARROW

%token _L_BRACKET
%token _L_DOT
%token _L_ANGLE
%token _L_SLASH
%token _L_HAIR
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

%start doc /* start rule */
%type <nd> tex
%type <nd> term
%type <nd> factor
%type <nd> pack
%type <nd> atom
%type <nd> script
%type <nd> s_atom
%type <nd> pair
%type <nd> mat_tex
%type <nd> rel
%type <nd> abv_tex

/* ======================================
 * associativity rules (order-sensitive)
 * =====================================*/
/* table/matrix has the lowest precedence */
%right TAB_ROW
%right TAB_COL

/* tex precedence */
%right _OVER
%right CHOOSE
%right SEP_CLASS
%right REL_CLASS
%right _STACKREL
%right _BUILDREL
%right _SET_REL
%right X_ARROW
%right ABOVE
%left  NULL_REDUCE
%left  ADD NEG
%left  _UNDER_OVER_BRACE /* should precede script */

/* factor precedence */
%nonassoc FACT
%nonassoc PRIME
%right    SUPSCRIPT SUBSCRIPT
%left     TIMES DIV

/* atom precedence */
%right    MODULAR
%right    BINOM
%right    FRAC
%right    VECT
%right    SQRT
%right    ROOT _OF
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
%nonassoc _L_VERT        _R_VERT

%%
doc: line | doc line;

line: tex '\n';

tex: %prec NULL_REDUCE {
	struct optr_node *nil = optr_alloc(S_NIL, T_NIL,
	                                   WC_NORMAL_LEAF);
	OPTR_ATTACH($$, NULL, NULL, nil);
}
| term {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| tex ADD term {
	OPTR_ATTACH($$, $1, $3, $2);
}
| tex ADD script term {
	OPTR_ATTACH($$, $1, $4, $2);
	optr_release($3);
}
| tex ADD {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| tex ADD script {
	OPTR_ATTACH($$, $1, NULL, $2);
	optr_release($3);
}
| tex NEG term {
	struct optr_node *neg, *add = optr_alloc(S_plus, T_ADD,
	                                         WC_COMMUT_OPERATOR);
	OPTR_ATTACH(neg, $3, NULL, $2);
	OPTR_ATTACH($$, neg, $1, add);
}
| tex NEG {
	OPTR_ATTACH($$, NULL, NULL, $1);
	optr_release($2);
}
| tex REL_CLASS tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| tex REL_CLASS script tex {
	optr_release($3);
	OPTR_ATTACH($$, $1, $4, $2);
}
| tex SEP_CLASS tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| tex SEP_CLASS script tex {
	optr_release($3);
	OPTR_ATTACH($$, $1, $4, $2);
}
| tex MODULAR tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| tex ABOVE tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| tex _OVER tex {
	struct optr_node *frac;
	frac = optr_alloc(S_frac, T_FRAC, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $1, $3, frac);
}
| tex CHOOSE tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
/* "stack-above" rules */
| tex _STACKREL atom rel tex {
	OPTR_ATTACH($$, $1, $5, $4);
	optr_release($3);
}
| tex _BUILDREL abv_tex _OVER rel tex {
	OPTR_ATTACH($$, $1, $6, $5);
	optr_release($3);
}
| tex _SET_REL atom rel tex {
	OPTR_ATTACH($$, $1, $5, $4);
	optr_release($3);
}
| tex X_ARROW atom tex {
	OPTR_ATTACH($$, $1, $4, $2);
	optr_release($3);
}
| tex X_ARROW _L_TEX_BRACKET abv_tex _R_TEX_BRACKET atom tex {
	OPTR_ATTACH($$, $1, $7, $2);
	optr_release($4);
	optr_release($6);
}
;

rel: atom {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| REL_CLASS {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
;

abv_tex: %prec NULL_REDUCE {
	struct optr_node *nil = optr_alloc(S_NIL, T_NIL,
	                                   WC_NORMAL_LEAF);
	OPTR_ATTACH($$, NULL, NULL, nil);
}
| term {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| abv_tex ADD term {
	OPTR_ATTACH($$, $1, $3, $2);
}
| abv_tex ADD {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| abv_tex NEG term {
	struct optr_node *neg, *add = optr_alloc(S_plus, T_ADD,
	                                         WC_COMMUT_OPERATOR);
	OPTR_ATTACH(neg, $3, NULL, $2);
	OPTR_ATTACH($$, neg, $1, add);
}
| abv_tex NEG {
	OPTR_ATTACH($$, NULL, NULL, $1);
	optr_release($2);
}
| abv_tex REL_CLASS abv_tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| abv_tex SEP_CLASS abv_tex {
	OPTR_ATTACH($$, $1, $3, $2);
}

mat_tex: %prec NULL_REDUCE {
	struct optr_node *nil = optr_alloc(S_NIL, T_NIL,
	                                   WC_NORMAL_LEAF);
	OPTR_ATTACH($$, NULL, NULL, nil);
}
| term {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| mat_tex ADD term {
	OPTR_ATTACH($$, $1, $3, $2);
}
| mat_tex ADD {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| mat_tex NEG term {
	struct optr_node *neg, *add = optr_alloc(S_plus, T_ADD,
	                                         WC_COMMUT_OPERATOR);
	OPTR_ATTACH(neg, $3, NULL, $2);
	OPTR_ATTACH($$, neg, $1, add);
}
| mat_tex NEG {
	OPTR_ATTACH($$, NULL, NULL, $1);
	optr_release($2);
}
| mat_tex REL_CLASS mat_tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| mat_tex SEP_CLASS mat_tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| mat_tex ABOVE mat_tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| mat_tex _OVER mat_tex {
	struct optr_node *frac;
	frac = optr_alloc(S_frac, T_FRAC, WC_NONCOM_OPERATOR);
	OPTR_ATTACH($$, $1, $3, frac);
}
| mat_tex TAB_ROW mat_tex {
	OPTR_ATTACH($$, $1, $3, $2);
}
| mat_tex TAB_COL mat_tex {
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
| term TIMES {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| term TIMES script {
	OPTR_ATTACH($$, $1, NULL, $2);
	optr_release($3);
}
| TIMES {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| TIMES script {
	OPTR_ATTACH($$, NULL, NULL, $1);
	optr_release($2);
}
| term DIV factor {
	OPTR_ATTACH($$, $1, $3, $2);
}
;

factor: pack {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| factor FACT {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| factor PRIME {
	OPTR_ATTACH($$, $1, NULL, $2);
}
| factor script {
	struct optr_node *base;
	base = optr_alloc(S_base, T_BASE, WC_COMMUT_OPERATOR);
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
| _BEGIN_MAT mat_tex _END_MAT {
	OPTR_ATTACH($$, NULL, NULL, $2);
}
;

atom: VAR {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| NUM {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| _L_TEX_BRACE tex _R_TEX_BRACE {
	/* tex braces is invisible in math rendering,
	 * and it is treated as atom. Think about that
	 * the command "\frac a {b+c}" works, but the
	 * command "\frac a (b+c)" does not. */
	OPTR_ATTACH($$, NULL, NULL, $2);
}
| _L_TEX_BRACE FACT _R_TEX_BRACE {
	OPTR_ATTACH($$, NULL, NULL, $2);
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
| BINOM__ {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| BINOM atom atom {
	OPTR_ATTACH($$, $2, $3, $1);
}
| VECT atom {
	OPTR_ATTACH($$, $2, NULL, $1);
}
| SQRT atom {
	OPTR_ATTACH($$, $2, NULL, $1);
}
| SQRT _L_TEX_BRACKET tex _R_TEX_BRACKET atom {
	/* example: \sqrt[3] 8 = 2 */
	OPTR_ATTACH($$, $5, $3, $1);
}
| ROOT atom _OF atom {
	/* example: \root 3 \of 8 = 2 */
	OPTR_ATTACH($$, $4, $2, $1);
}
| _QVAR VAR {
	struct optr_node *var = $2;
	var->wildcard = true;
	OPTR_ATTACH($$, NULL, NULL, $2);
}
| _QVAR _L_TEX_BRACE VAR _R_TEX_BRACE {
	struct optr_node *var = $3;
	var->wildcard = true;
	OPTR_ATTACH($$, NULL, NULL, $3);
}
| _QVAR _L_TEX_BRACE NUM _R_TEX_BRACE {
	struct optr_node *var = $3;
	var->wildcard = true;
	OPTR_ATTACH($$, NULL, NULL, $3);
}
| _UNDER_OVER_BRACE atom script {
	OPTR_ATTACH($$, NULL, NULL, $2);
	optr_release($3);
}
| _UNDER_OVER_BRACE atom {
	OPTR_ATTACH($$, NULL, NULL, $2);
}
;

s_atom: atom {
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| ADD {
	struct optr_node *var = $1;
	var->wildcard = false;
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| NEG {
	struct optr_node *var = $1;
	var->wildcard = false;
	OPTR_ATTACH($$, NULL, NULL, $1);
}
| TIMES {
	struct optr_node *var = $1;
	var->wildcard = false;
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
	pair = optr_alloc(S_bracket, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
/* bracket array */
| _L_DOT tex _R_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_array, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_BRACKET tex _R_DOT {
	struct optr_node *pair;
	pair = optr_alloc(S_array, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
/* range */
| _L_BRACKET tex _R_TEX_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_bracket, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_TEX_BRACKET tex _R_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_bracket, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
/* others */
| _L_ANGLE tex _R_ANGLE {
	struct optr_node *pair;
	pair = optr_alloc(S_angle, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_ANGLE tex _R_DOT {
	struct optr_node *pair;
	pair = optr_alloc(S_angle, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_ANGLE tex _R_VERT {
	struct optr_node *pair;
	pair = optr_alloc(S_angle, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_VERT tex _R_ANGLE {
	struct optr_node *pair;
	pair = optr_alloc(S_angle, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_DOT tex _R_ANGLE {
	struct optr_node *pair;
	pair = optr_alloc(S_angle, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_SLASH tex _R_SLASH {
	struct optr_node *pair;
	pair = optr_alloc(S_slash, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_HAIR tex _R_HAIR {
	struct optr_node *pair;
	pair = optr_alloc(S_hair, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_ARROW tex _R_ARROW {
	struct optr_node *pair;
	pair = optr_alloc(S_arrow, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_TEX_BRACKET tex _R_TEX_BRACKET {
	struct optr_node *pair;
	pair = optr_alloc(S_bracket, T_GROUP, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_CEIL tex _R_CEIL {
	struct optr_node *pair;
	pair = optr_alloc(S_ceil, T_CEIL, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_FLOOR tex _R_FLOOR {
	struct optr_node *pair;
	pair = optr_alloc(S_floor, T_FLOOR, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_VERT tex _R_VERT {
	struct optr_node *pair;
	pair = optr_alloc(S_verts, T_VERTS, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_VERT tex _R_DOT {
	struct optr_node *pair;
	pair = optr_alloc(S_verts, T_VERTS, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
| _L_DOT tex _R_VERT {
	struct optr_node *pair;
	pair = optr_alloc(S_verts, T_VERTS, WC_COMMUT_OPERATOR);
	OPTR_ATTACH($$, $2, NULL, pair);
}
;
%%

/* yyerror shows grammar error */
int yyerror(const char *msg)
{
	strcpy(grammar_last_err_str, msg);

	/* set root to NULL to avoid double free */
	grammar_optr_root = NULL;
	grammar_err_flag = 1;

	return 0;
}
