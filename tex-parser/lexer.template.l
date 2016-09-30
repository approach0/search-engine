%{
#include "head.h"
#include "y.tab.h"

#define RET_TOK(_symbol, _token, _wc, _gram_token) \
	yylval.nd = optr_alloc(S_ ## _symbol, T_ ## _token, _wc); \
	return _gram_token;

#define LVAL (&yylval.nd)

#define RET_HACK(_symbol, _token, _wc, _gram_token) \
	return hack_attach(optr_alloc(S_ ## _symbol, T_ ## _token, _wc), \
	                              yytext, LVAL, _gram_token);

int ret_var(char*, struct optr_node **, char);
int ret_num(char*, struct optr_node **);
int ret_float_num(char*, struct optr_node **);
int hack_attach(struct optr_node*, char*, struct optr_node **, int);

extern unsigned int ign_stack;
extern unsigned int mat_stack;
extern int lexer_warning_flag;
%}
 /* ==================
  *  start conditions
  * =================*/

 /* for dimension (include cm, mm, in and pt) */
%x dim
 /* command (\example{...}) to be ignored */
%x ign
 /* TeX comments (begin with a "%") */
%x cmt
 /* matrix domain */
%s mat

%%
 /* ============
  *  comments
  * ===========*/
"%"                                           { BEGIN(cmt); }
<cmt>\n                      { BEGIN(INITIAL); return '\n'; }
<cmt>.                                       { /* ignore */ }

 /* ========================
  *  commands to be ignored
  * ========================*/
\\color\{                       { BEGIN(ign); ign_stack ++; }
\\mbox\{                        { BEGIN(ign); ign_stack ++; }
\\hbox\{                        { BEGIN(ign); ign_stack ++; }
\\label\{                       { BEGIN(ign); ign_stack ++; }
\\tag\{                         { BEGIN(ign); ign_stack ++; }
\\text\{                        { BEGIN(ign); ign_stack ++; }
\\leftroot\{                    { BEGIN(ign); ign_stack ++; }
\\uproot\{                      { BEGIN(ign); ign_stack ++; }

<ign>\{                         { ign_stack ++; }
<ign>\}     { ign_stack --; if (!ign_stack) BEGIN(INITIAL); }
<ign>.|\n                                                  {}

 /* =============
  *  add/minus
  * ============*/
"+"                                { RET_TOK(plus,      ADD, WC_COMMUT_OPERATOR, ADD); }
"±"                                { RET_TOK(pm,        ADD, WC_COMMUT_OPERATOR, ADD); }
\\oplus                            { RET_TOK(_AUTOGEN_, ADD, WC_COMMUT_OPERATOR, ADD); }
\\uplus                            { RET_TOK(_AUTOGEN_, ADD, WC_COMMUT_OPERATOR, ADD); }
\\dotplus                          { RET_TOK(_AUTOGEN_, ADD, WC_COMMUT_OPERATOR, ADD); }
\\pm                               { RET_TOK(_AUTOGEN_, ADD, WC_COMMUT_OPERATOR, ADD); }
\\mp                               { RET_TOK(_AUTOGEN_, ADD, WC_COMMUT_OPERATOR, ADD); }
"-"                                { RET_TOK(minus,     NEG, WC_COMMUT_OPERATOR, NEG); }
\\neg                              { RET_TOK(_AUTOGEN_, NEG, WC_COMMUT_OPERATOR, NEG); }
\\ominus                           { RET_TOK(_AUTOGEN_, NEG, WC_COMMUT_OPERATOR, NEG); }
\\setminus                         { RET_TOK(_AUTOGEN_, NEG, WC_COMMUT_OPERATOR, NEG); }
\\backslash                        { RET_TOK(_AUTOGEN_, NEG, WC_COMMUT_OPERATOR, NEG); }
\\smallsetminus                    { RET_TOK(_AUTOGEN_, NEG, WC_COMMUT_OPERATOR, NEG); }
\\lnot                             { RET_TOK(_AUTOGEN_, NEG, WC_COMMUT_OPERATOR, NEG); }
\\barwedge                         { RET_TOK(_AUTOGEN_, NEG, WC_COMMUT_OPERATOR, NEG); }

 /* ==================
  *  star and times
  * ================*/
\\ast                                 { RET_TOK(_AUTOGEN_, STAR, WC_NORMAL_LEAF, VAR); }
"*"                                         { RET_TOK(ast, STAR, WC_NORMAL_LEAF, VAR); }
\\times                            { RET_TOK(times, TIMES, WC_COMMUT_OPERATOR, TIMES); }
\\cdot                             { RET_TOK(times, TIMES, WC_COMMUT_OPERATOR, TIMES); }
\\otimes                       { RET_TOK(_AUTOGEN_, TIMES, WC_COMMUT_OPERATOR, TIMES); }
\\ltimes                       { RET_TOK(_AUTOGEN_, TIMES, WC_COMMUT_OPERATOR, TIMES); }
\\rtimes                       { RET_TOK(_AUTOGEN_, TIMES, WC_COMMUT_OPERATOR, TIMES); }
\\odot                         { RET_TOK(_AUTOGEN_, TIMES, WC_COMMUT_OPERATOR, TIMES); }

 /* =====================
  *  div and fraction
  * ===================*/
 /* div */
\/                                     { RET_TOK(frac, FRAC, WC_NONCOM_OPERATOR, DIV); }
\\div                                  { RET_TOK(frac, FRAC, WC_NONCOM_OPERATOR, DIV); }
\\divideontimes                   { RET_TOK(_AUTOGEN_, FRAC, WC_NONCOM_OPERATOR, DIV); }

 /* over */
\\over                                                                 { return _OVER; }

 /* \above command example:
  * {a \above 2pt b + c}
  * it needs a dimension (e.g. cm, mm and pt) argument right after,
  * we need to filter this argument out since it is irrelevant to
  * math semantics.
  */
\\above                  { BEGIN(dim); RET_TOK(frac, FRAC, WC_NONCOM_OPERATOR, ABOVE); }

 /* frac */
\\frac                                { RET_TOK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC); }
\\dfrac                               { RET_TOK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC); }
\\cfrac                               { RET_TOK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC); }
\\tfrac                               { RET_TOK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC); }
\\frac[ ]*[0-9][0-9]               { RET_HACK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC__); }
\\dfrac[ ]*[0-9][0-9]              { RET_HACK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC__); }
\\cfrac[ ]*[0-9][0-9]              { RET_HACK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC__); }
\\tfrac[ ]*[0-9][0-9]              { RET_HACK(frac, FRAC, WC_NONCOM_OPERATOR, FRAC__); }

 /* ============================
  *  dimension condition state
  * ===========================*/
\\hspace                                                                 { BEGIN(dim); }
\\hskip                                                                  { BEGIN(dim); }
\\mspace                                                                 { BEGIN(dim); }
\\mskip                                                                  { BEGIN(dim); }
\\mkern                                                                  { BEGIN(dim); }
\\kern                                                                   { BEGIN(dim); }

<dim>em\}?                                                           { BEGIN(INITIAL); }
<dim>ex\}?                                                           { BEGIN(INITIAL); }
<dim>pt\}?                                                           { BEGIN(INITIAL); }
<dim>pc\}?                                                           { BEGIN(INITIAL); }
<dim>mu\}?                                                           { BEGIN(INITIAL); }
<dim>cm\}?                                                           { BEGIN(INITIAL); }
<dim>mm\}?                                                           { BEGIN(INITIAL); }
<dim>in\}?                                                           { BEGIN(INITIAL); }
<dim>px\}?                                                           { BEGIN(INITIAL); }
<dim>.|\n                                                                             {}

 /* ===============
  *  table/matrix
  * ===============*/
\\begin\{matrix\}                       { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }
\\begin\{vmatrix\}                      { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }
\\begin\{Vmatrix\}                      { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }
\\begin\{bmatrix\}                      { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }
\\begin\{Bmatrix\}                      { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }
\\begin\{pmatrix\}                      { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }
\\begin\{smallmatrix\}                  { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }
\\begin\{cases\}                        { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }

\\end\{matrix\}                       { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 
\\end\{vmatrix\}                      { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 
\\end\{Vmatrix\}                      { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 
\\end\{bmatrix\}                      { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 
\\end\{Bmatrix\}                      { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 
\\end\{pmatrix\}                      { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 
\\end\{smallmatrix\}                  { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 
\\end\{cases\}                        { BEGIN(INITIAL); mat_stack ++; return _END_MAT; } 

\\array\{                               { BEGIN(mat); mat_stack ++; return _BEGIN_MAT; }

<mat>"{"                                             { mat_stack ++; return yytext[0]; }

<mat>\\\\                        { RET_TOK(row, TAB_ROW, WC_COMMUT_OPERATOR, TAB_ROW); }
<mat>\\cr                        { RET_TOK(row, TAB_ROW, WC_COMMUT_OPERATOR, TAB_ROW); }
<mat>\\newline                   { RET_TOK(row, TAB_ROW, WC_COMMUT_OPERATOR, TAB_ROW); }
<mat>&                        { RET_TOK(column, TAB_COL, WC_COMMUT_OPERATOR, TAB_COL); }

<mat>"}" { 
	mat_stack --; 
	if (!mat_stack) { 
		BEGIN(INITIAL);
		return _END_MAT; 
	} else {
		return yytext[0]; 
	}
}

 /* ==========
  * binomials
  * =========*/
 /* example: {a \choose b + c} */
\\choose                          { RET_TOK(binom, BINOM, WC_NONCOM_OPERATOR, CHOOSE); }
 /* example: {a \brack b + c} */
\\brack                           { RET_TOK(binom, BINOM, WC_NONCOM_OPERATOR, CHOOSE); }

\\dbinom[ ]*[0-9][0-9]          { RET_HACK(binom, BINOM, WC_NONCOM_OPERATOR, BINOM__); }
\\tbinom[ ]*[0-9][0-9]          { RET_HACK(binom, BINOM, WC_NONCOM_OPERATOR, BINOM__); }
\\binom[ ]*[0-9][0-9]           { RET_HACK(binom, BINOM, WC_NONCOM_OPERATOR, BINOM__); }
\\dbinom                           { RET_TOK(binom, BINOM, WC_NONCOM_OPERATOR, BINOM); }
\\tbinom                           { RET_TOK(binom, BINOM, WC_NONCOM_OPERATOR, BINOM); }
\\binom                            { RET_TOK(binom, BINOM, WC_NONCOM_OPERATOR, BINOM); }

 /* ===============
  *  sqrt and root
  * ==============*/
\\sqrt                                { RET_TOK(root, ROOT, WC_NONCOM_OPERATOR, SQRT); }
\\root                                { RET_TOK(root, ROOT, WC_NONCOM_OPERATOR, ROOT); }
\\of                                                                     { return _OF; }

 /* ===============
  *  modular
  * ==============*/
\\pmod                           { RET_TOK(mod, MODULAR, WC_NONCOM_OPERATOR, MODULAR); }
\\bmod                           { RET_TOK(mod, MODULAR, WC_NONCOM_OPERATOR, MODULAR); }
\\mod                            { RET_TOK(mod, MODULAR, WC_NONCOM_OPERATOR, MODULAR); }
\\pod                            { RET_TOK(mod, MODULAR, WC_NONCOM_OPERATOR, MODULAR); }

 /* =========
  *  vector
  * ========*/
\\vec                                { RET_TOK(rvect, VECT, WC_NONCOM_OPERATOR, VECT); }
\\overrightarrow                     { RET_TOK(rvect, VECT, WC_NONCOM_OPERATOR, VECT); }
\\overleftarrow                      { RET_TOK(lvect, VECT, WC_NONCOM_OPERATOR, VECT); }

 /* ===========
  * factorial
  * ==========*/
"!"                                   { RET_TOK(fact, FACT, WC_NONCOM_OPERATOR, FACT); }

 /* =====================
  *  groups and pairs
  * ===================*/
 /* left long match */
"("|"\\{"                                                         { return _L_BRACKET; }
\\left[ ]*"."                                                         { return _L_DOT; }
\\left[ ]*"("                                                     { return _L_BRACKET; }
\\left[ ]*\\lgroup                                                { return _L_BRACKET; }
\\left[ ]*"["                                                     { return _L_BRACKET; }
\\left[ ]*"\{"                                                    { return _L_BRACKET; }
\\left[ ]*\\langle                                                  { return _L_ANGLE; }
\\left[ ]*"<"                                                       { return _L_ANGLE; }
\\left[ ]*"/"                                                       { return _L_SLASH; }
\\left[ ]*"\\"                                                      { return _L_SLASH; }
\\left[ ]*\\backslash                                               { return _L_SLASH; }
\\left[ ]*\\lmoustache                                               { return _L_HAIR; }
\\left[ ]*\\[Uu]parrow                                              { return _L_ARROW; }
\\left[ ]*\\[Dd]ownarrow                                            { return _L_ARROW; }
\\left[ ]*\\[Uu]pdownarrow                                          { return _L_ARROW; }
 /* left short match */
\\left                                                                { return _L_DOT; }
\\lgroup                                                          { return _L_BRACKET; }
\\langle                                                            { return _L_ANGLE; }
\\lmoustache                                                         { return _L_HAIR; }
\\lbrace                                                          { return _L_BRACKET; }
\\lbrack                                                          { return _L_BRACKET; }

 /* right long match */
")"|"\\}"                                                         { return _R_BRACKET; }
\\right[ ]*"."                                                        { return _R_DOT; }
\\right[ ]*")"                                                    { return _R_BRACKET; }
\\right[ ]*\\rgroup                                               { return _R_BRACKET; }
\\right[ ]*"]"                                                    { return _R_BRACKET; }
\\right[ ]*"\}"                                                   { return _R_BRACKET; }
\\right[ ]*\\rangle                                                 { return _R_ANGLE; }
\\right[ ]*">"                                                      { return _R_ANGLE; }
\\right[ ]*"/"                                                      { return _R_SLASH; }
\\right[ ]*"\\"                                                     { return _R_SLASH; }
\\right[ ]*\\backslash                                              { return _R_SLASH; }
\\right[ ]*\\rmoustache                                              { return _R_HAIR; }
\\right[ ]*\\[Uu]parrow                                             { return _R_ARROW; }
\\right[ ]*\\[Dd]ownarrow                                           { return _R_ARROW; }
\\right[ ]*\\[Uu]pdownarrow                                         { return _R_ARROW; }
 /* right short match */
\\right                                                               { return _R_DOT; }
\\rgroup                                                          { return _R_BRACKET; }
\\rangle                                                            { return _R_ANGLE; }
\\rmoustache                                                         { return _R_HAIR; }
\\rbrace                                                          { return _R_BRACKET; }
\\rbrack                                                          { return _R_BRACKET; }

 /* tex bracket */
"{"                                                             { return _L_TEX_BRACE; }
"}"                                                             { return _R_TEX_BRACE; }
"["                                                           { return _L_TEX_BRACKET; }
"]"                                                           { return _R_TEX_BRACKET; }

 /* ceil and floor */
\\left[ ]*\\lceil                                                    { return _L_CEIL; }
\\left[ ]*\\lfloor                                                  { return _L_FLOOR; }
\\right[ ]*\\rceil                                                   { return _R_CEIL; }
\\right[ ]*\\rfloor                                                 { return _R_FLOOR; }
 /* shorter ceil and floor */
\\lceil                                                              { return _L_CEIL; }
\\lfloor                                                            { return _L_FLOOR; }
\\rceil                                                              { return _R_CEIL; }
\\rfloor                                                            { return _R_FLOOR; }

 /* absolute value */
\\left[ ]*\|                                                         { return _L_VERT; }
\\right[ ]*\|                                                        { return _R_VERT; }

 /* =========================
  * primes and su[bp]scripts
  * ========================*/
"'"                                { RET_TOK(prime, PRIME, WC_NONCOM_OPERATOR, PRIME); }
\\prime                              { RET_TOK(_AUTOGEN_, PRIME, WC_NORMAL_LEAF, VAR); }
"_"                    { RET_TOK(subscript, SUBSCRIPT, WC_COMMUT_OPERATOR, SUBSCRIPT); }
"^"                    { RET_TOK(supscript, SUPSCRIPT, WC_COMMUT_OPERATOR, SUPSCRIPT); }

 /* =================
  *  numbers
  * ================*/
[0-9]+\.[0-9]*                                   { return ret_float_num(yytext, LVAL); }
[0-9]+                                                 { return ret_num(yytext, LVAL); }

 /* =============
  *  wildcard 
  * ============*/
\\qvar                                                                 { return _QVAR; }

 /* =============
  *  variables
  * ============*/

 /* single letter variables */
[a-z]                                             { return ret_var(yytext, LVAL, 'a'); }
[A-Z]                                        { return ret_var(yytext, LVAL, 'A' - 26); }

 /* unicode variables */
α                                          { RET_TOK(alpha, VAR, WC_NORMAL_LEAF, VAR); }
β                                          { RET_TOK(beta,  VAR, WC_NORMAL_LEAF, VAR); }

 /* auto-generated discriminative tokens, where NIL is to be 
  * replaced by specially assigned symbol. */
\\Alpha                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Beta                                 { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Chi                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Delta                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Epsilon                              { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Eta                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Gamma                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Iota                                 { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Kappa                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Lambda                               { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Mu                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Nu                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Omega                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Omicron                              { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Phi                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Pi                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Psi                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Re                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Rho                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Sigma                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Tau                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Theta                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Upsilon                              { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\VarLambda                            { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\VarOmega                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Xi                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\Zeta                                 { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\aleph                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\alpha                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\amalg                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\beta                                 { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\beth                                 { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\chi                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\delta                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\ell                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\epsilon                              { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\eta                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\eth                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\gamma                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\imath                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\iota                                 { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\jmath                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\kappa                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\lambda                               { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\mho                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\mu                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\nu                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\omega                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\omicron                              { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\phi                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\psi                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\rho                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\sigma                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\tau                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\theta                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\top                                  { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\upsilon                              { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varDelta                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varGamma                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varPhi                               { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varPi                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varPsi                               { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varSigma                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varTheta                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varUpsilon                           { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varXi                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varepsilon                           { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varkappa                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varphi                               { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varpi                                { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varpropto                            { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varrho                               { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\varsigma                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\vartheta                             { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\wr                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\xi                                   { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\zeta                                 { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }
\\backepsilon                          { RET_TOK(_AUTOGEN_, VAR, WC_NORMAL_LEAF, VAR); }

 /* ==================================
  *  miscellaneous special variables
  * =================================*/
 /* partial */
\\partial                          { RET_TOK(_AUTOGEN_, PARTIAL, WC_NORMAL_LEAF, VAR); }
\\nabla                            { RET_TOK(_AUTOGEN_, PARTIAL, WC_NORMAL_LEAF, VAR); }

 /* PI */
\\pi                                    { RET_TOK(_AUTOGEN_, PI, WC_NORMAL_LEAF, VAR); }

 /* infty */
\\infty                              { RET_TOK(_AUTOGEN_, INFTY, WC_NORMAL_LEAF, VAR); }
∞                                        { RET_TOK(infty, INFTY, WC_NORMAL_LEAF, VAR); }

 /* empty */
\\empty                              { RET_TOK(_AUTOGEN_, EMPTY, WC_NORMAL_LEAF, VAR); }
\\emptyset                           { RET_TOK(_AUTOGEN_, EMPTY, WC_NORMAL_LEAF, VAR); }
\\varnothing                         { RET_TOK(_AUTOGEN_, EMPTY, WC_NORMAL_LEAF, VAR); }

 /* angle */
\\triangledown                       { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\triangle                           { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\angle                              { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\vartriangleleft                    { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\vartriangleright                   { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\vartriangle                        { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\triangleleft                       { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\triangleright                      { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\measuredangle                      { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }
\\sphericalangle                     { RET_TOK(_AUTOGEN_, ANGLE, WC_NORMAL_LEAF, VAR); }

 /* perpendicular */
\\perp                                { RET_TOK(_AUTOGEN_, PERP, WC_NORMAL_LEAF, VAR); }
\\bot                                 { RET_TOK(_AUTOGEN_, PERP, WC_NORMAL_LEAF, VAR); }

 /* circle */
\\circ                                { RET_TOK(_AUTOGEN_, CIRC, WC_NORMAL_LEAF, VAR); }

 /* percentage */
\\%                                  { RET_TOK(percent, PERCENT, WC_NORMAL_LEAF, VAR); }

 /* long dots */
\.\.\.                                     { RET_TOK(dots, DOTS, WC_NORMAL_LEAF, VAR); }
\\dots                                { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\ldots                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\vdots                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\cdots                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\ddots                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\ddot                                { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\dddot                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\ddddot                              { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\dotsb                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\dotsc                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\dotsi                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\dotsm                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\dotso                               { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }
\\iddots                              { RET_TOK(_AUTOGEN_, DOTS, WC_NORMAL_LEAF, VAR); }

 /* vertical lines */
"|"                                        { RET_TOK(vert, VERT, WC_NORMAL_LEAF, VAR); }
"\\|"                                      { RET_TOK(Vert, VERT, WC_NORMAL_LEAF, VAR); }
\\vert                                { RET_TOK(_AUTOGEN_, VERT, WC_NORMAL_LEAF, VAR); }
\\Vert                                { RET_TOK(_AUTOGEN_, VERT, WC_NORMAL_LEAF, VAR); }
\\Arrowvert                           { RET_TOK(_AUTOGEN_, VERT, WC_NORMAL_LEAF, VAR); }
\\arrowvert                           { RET_TOK(_AUTOGEN_, VERT, WC_NORMAL_LEAF, VAR); }
\\bracevert                           { RET_TOK(_AUTOGEN_, VERT, WC_NORMAL_LEAF, VAR); }
\\rvert                                    { RET_TOK(vert, VERT, WC_NORMAL_LEAF, VAR); }
\\lvert                                    { RET_TOK(vert, VERT, WC_NORMAL_LEAF, VAR); }
\\rVert                                    { RET_TOK(Vert, VERT, WC_NORMAL_LEAF, VAR); }
\\lVert                                    { RET_TOK(Vert, VERT, WC_NORMAL_LEAF, VAR); }
\\mid                                 { RET_TOK(_AUTOGEN_, VERT, WC_NORMAL_LEAF, VAR); }
\\nmid                                { RET_TOK(_AUTOGEN_, VERT, WC_NORMAL_LEAF, VAR); }

 /* ======================
  *  seperation class
  * ====================*/
 /* forall and exists */
\\exists                  { RET_TOK(_AUTOGEN_, EXISTS, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\nexists                 { RET_TOK(_AUTOGEN_, EXISTS, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\forall                  { RET_TOK(_AUTOGEN_, FORALL, WC_NONCOM_OPERATOR, SEP_CLASS); }

 /* classic seperator */ 
\\cr                         { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\newline                    { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\\\                           { RET_TOK(newline, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\enspace                    { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\atop                       { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
,                                { RET_TOK(comma, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
;                            { RET_TOK(semicolon, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\colon                       { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
":"                              { RET_TOK(colon, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\:                              { RET_TOK(colon, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\And                        { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\\&                               { RET_TOK(And, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\qquad                      { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }
\\quad                       { RET_TOK(_AUTOGEN_, SEP, WC_NONCOM_OPERATOR, SEP_CLASS); }

 /* arrows */
\\to                       { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\searrow                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\uparrow                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\updownarrow              { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\upharpoonleft            { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\upharpoonright           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\upuparrows               { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Leftarrow                { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Leftrightarrow           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Lleftarrow               { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Longleftarrow            { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Longleftrightarrow       { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Longrightarrow           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Lsh                      { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Rightarrow               { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Rrightarrow              { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Rsh                      { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Uparrow                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Updownarrow              { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\circlearrowleft          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\circlearrowright         { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\curvearrowleft           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\curvearrowright          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\Downarrow                { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\downarrow                { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\downdownarrows           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\downharpoonleft          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\downharpoonright         { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\hookleftarrow            { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\hookrightarrow           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\gets                     { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\iff                      { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\impliedby                { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\implies                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftarrow                { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftarrowtail            { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftharpoondown          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftharpoonup            { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftleftarrows           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftrightarrow           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftrightarrows          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftrightharpoons        { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leftrightsquigarrow      { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\longleftarrow            { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\longleftrightarrow       { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\longmapsto               { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\longrightarrow           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\looparrowleft            { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\looparrowright           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\mapsto                   { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\multimap                 { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nLeftarrow               { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nLeftrightarrow          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nRightarrow              { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nearrow                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nleftarrow               { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nleftrightarrow          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nrightarrow              { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\nwarrow                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightarrow               { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightarrowtail           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightharpoondown         { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightharpoonup           { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightleftarrows          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightleftharpoons        { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightrightarrows         { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\rightsquigarrow          { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\swarrow                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }
\\leadsto                  { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, SEP_CLASS); }

 /* ==================
  *  function class
  * ================*/
\\exp                            { RET_TOK(_AUTOGEN_, EXP, WC_NORMAL_LEAF, FUN_CLASS); }

\\lg                             { RET_TOK(_AUTOGEN_, LOG, WC_NORMAL_LEAF, FUN_CLASS); }
\\ln                             { RET_TOK(_AUTOGEN_, LOG, WC_NORMAL_LEAF, FUN_CLASS); }
\\log                            { RET_TOK(_AUTOGEN_, LOG, WC_NORMAL_LEAF, FUN_CLASS); }

\\sin                  { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\sinh                 { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\arcsin               { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\cos                  { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\arccos               { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\cosh                 { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\tan                  { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\tanh                 { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\arctan               { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\cot                  { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\coth                 { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\csc                  { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }
\\sec                  { RET_TOK(_AUTOGEN_, TRIGONOMETRIC, WC_NORMAL_LEAF, FUN_CLASS); }

 /* the three below are not LaTeX standard commands, but appear occasionally. */
\\sgn                           { RET_TOK(_AUTOGEN_, SIGN, WC_NORMAL_LEAF, FUN_CLASS); }
\\signum                        { RET_TOK(_AUTOGEN_, SIGN, WC_NORMAL_LEAF, FUN_CLASS); }
\\sign                          { RET_TOK(_AUTOGEN_, SIGN, WC_NORMAL_LEAF, FUN_CLASS); }

\\max                         { RET_TOK(_AUTOGEN_, MAXMIN, WC_NORMAL_LEAF, FUN_CLASS); }
\\min                         { RET_TOK(_AUTOGEN_, MAXMIN, WC_NORMAL_LEAF, FUN_CLASS); }

\\Pr                        { RET_TOK(_AUTOGEN_, NAME_FUN, WC_NORMAL_LEAF, FUN_CLASS); }
\\deg                       { RET_TOK(_AUTOGEN_, NAME_FUN, WC_NORMAL_LEAF, FUN_CLASS); }
\\det                       { RET_TOK(_AUTOGEN_, NAME_FUN, WC_NORMAL_LEAF, FUN_CLASS); }
\\dim                       { RET_TOK(_AUTOGEN_, NAME_FUN, WC_NORMAL_LEAF, FUN_CLASS); }
\\gcd                       { RET_TOK(_AUTOGEN_, NAME_FUN, WC_NORMAL_LEAF, FUN_CLASS); }
\\hom                       { RET_TOK(_AUTOGEN_, NAME_FUN, WC_NORMAL_LEAF, FUN_CLASS); }
\\ker                       { RET_TOK(_AUTOGEN_, NAME_FUN, WC_NORMAL_LEAF, FUN_CLASS); }

 /* user-defined operator */
\\operatorname\*?\{[^}]*\}     { RET_TOK(usr_fun, USR_FUN, WC_NORMAL_LEAF, FUN_CLASS); }

 /* ==================
  *  sum class
  * ================*/
\\arg                            { RET_TOK(_AUTOGEN_, ARG, WC_NORMAL_LEAF, SUM_CLASS); }

\\inf                         { RET_TOK(_AUTOGEN_, INFSUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\sup                         { RET_TOK(_AUTOGEN_, INFSUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\liminf                      { RET_TOK(_AUTOGEN_, INFSUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\limsup                      { RET_TOK(_AUTOGEN_, INFSUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\varliminf                   { RET_TOK(_AUTOGEN_, INFSUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\varlimsup                   { RET_TOK(_AUTOGEN_, INFSUP, WC_NORMAL_LEAF, SUM_CLASS); }

\\bigcap                     { RET_TOK(_AUTOGEN_, BCAPCUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigcup                     { RET_TOK(_AUTOGEN_, BCAPCUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigsqcup                   { RET_TOK(_AUTOGEN_, BCAPCUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\biguplus                   { RET_TOK(_AUTOGEN_, BCAPCUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigvee                     { RET_TOK(_AUTOGEN_, BCAPCUP, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigwedge                   { RET_TOK(_AUTOGEN_, BCAPCUP, WC_NORMAL_LEAF, SUM_CLASS); }

\\bigcirc                       { RET_TOK(_AUTOGEN_, BIGO, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigodot                       { RET_TOK(_AUTOGEN_, BIGO, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigoplus                      { RET_TOK(_AUTOGEN_, BIGO, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigotimes                     { RET_TOK(_AUTOGEN_, BIGO, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigtriangledown               { RET_TOK(_AUTOGEN_, BIGO, WC_NORMAL_LEAF, SUM_CLASS); }
\\bigtriangleup                 { RET_TOK(_AUTOGEN_, BIGO, WC_NORMAL_LEAF, SUM_CLASS); }

\\sum                            { RET_TOK(_AUTOGEN_, SUM, WC_NORMAL_LEAF, SUM_CLASS); }

\\prod                          { RET_TOK(_AUTOGEN_, PROD, WC_NORMAL_LEAF, SUM_CLASS); }
\\coprod                        { RET_TOK(_AUTOGEN_, PROD, WC_NORMAL_LEAF, SUM_CLASS); }

\\lim                            { RET_TOK(_AUTOGEN_, LIM, WC_NORMAL_LEAF, SUM_CLASS); }
\\injlim                         { RET_TOK(_AUTOGEN_, LIM, WC_NORMAL_LEAF, SUM_CLASS); }
\\varinjlim                      { RET_TOK(_AUTOGEN_, LIM, WC_NORMAL_LEAF, SUM_CLASS); }
\\varprojlim                     { RET_TOK(_AUTOGEN_, LIM, WC_NORMAL_LEAF, SUM_CLASS); }
\\projlim                        { RET_TOK(_AUTOGEN_, LIM, WC_NORMAL_LEAF, SUM_CLASS); }

\\idotsint                       { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }
\\int                            { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }
\\iint                           { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }
\\iiint                          { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }
\\iiiint                         { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }
\\intop                          { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }
\\smallint                       { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }
\\oint                           { RET_TOK(_AUTOGEN_, INT, WC_NORMAL_LEAF, SUM_CLASS); }

 /* ============================
  *  relational calss
  * ===========================*/

 /* commutative gt/lt/eq */
=                               { RET_TOK(equal, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
:=                              { RET_TOK(equal, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\[dD]oteq                      { RET_TOK(equal, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\dot=                          { RET_TOK(equal, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\approxeq                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\backsimeq                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\circeq                    { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\cong                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\backsim                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\curlyeqprec               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\curlyeqsucc               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\eqslantgtr                { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\eqslantless               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\equiv                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gnsim                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\triangleq                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\eqsim                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\thicksim                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\sim                       { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\simeq                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nsim                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\neq                       { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\not("="|"\equiv")               { RET_TOK(neq, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\frown                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\between                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\eqcirc                    { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\smallfrown                { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\smallsmile                { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }

 /* non-commutative gt/lt/eq */
\\approx                    { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\asymp                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ge                        { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\geq                       { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\geqq                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\geqslant                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gg                        { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gnapprox                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gt                        { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
">"                                { RET_TOK(gt, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gtrapprox                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gtrdot                    { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gtreqless                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gtreqqless                { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gtrless                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gtrsim                    { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\le                        { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\leq                       { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\leqq                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\leqslant                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lessapprox                { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lessdot                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lesssim                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ll                        { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lnapprox                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lneq                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lneqq                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lt                        { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
"<"                                { RET_TOK(lt, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lvertneqq                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ncong                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ne                        { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ngeq                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ngeqq                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ngeqslant                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nleq                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nleqq                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nleqslant                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nless                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nprec                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\npreceq                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nsucc                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nsucceq                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\prec                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\preceq                    { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succ                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succapprox                { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succcurlyeq               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\thickapprox               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\trianglelefteq            { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\trianglerighteq           { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succeq                    { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succnapprox               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succneqq                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succnsim                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\succsim                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\unlhd                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\unrhd                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gneq                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gneqq                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gvertneqq                 { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ggg                       { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\gggtr                     { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ngtr                      { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\precapprox                { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\preccurlyeq               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\precnapprox               { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\precneqq                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\precnsim                  { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }
\\precsim                   { RET_TOK(_AUTOGEN_, GTLS, WC_COMMUT_OPERATOR, REL_CLASS); }

 /* cap/cup */
\\Cap                     { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\cap                     { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\Cup                     { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\cup                     { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\curlyvee                { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\dashv                   { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\curlywedge              { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\land                    { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\lor                     { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\sqcap                   { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\sqcup                   { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\vee                     { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\veebar                  { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }
\\wedge                   { RET_TOK(_AUTOGEN_, CAPCUP, WC_COMMUT_OPERATOR, REL_CLASS); }

 /* bowtie */
\\Join                    { RET_TOK(_AUTOGEN_, BOWTIE, WC_COMMUT_OPERATOR, REL_CLASS); }
\\bowtie                  { RET_TOK(_AUTOGEN_, BOWTIE, WC_COMMUT_OPERATOR, REL_CLASS); }

 /* subset */
\\Subset                  { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\Supset                  { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nsubseteq               { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\nsupseteq               { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\subset                  { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\sqsubset                { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\sqsubseteq              { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\sqsupset                { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\sqsupseteq              { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\subseteq                { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\subseteqq               { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\subsetneq               { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\subsetneqq              { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\supset                  { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\supseteq                { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\supseteqq               { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\supsetneq               { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\supsetneqq              { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\varsubsetneq            { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\varsubsetneqq           { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\varsupsetneq            { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }
\\varsupsetneqq           { RET_TOK(_AUTOGEN_, SUBSET, WC_COMMUT_OPERATOR, REL_CLASS); }

 /* in */
\\in                        { RET_TOK(_AUTOGEN_, INNI, WC_COMMUT_OPERATOR, REL_CLASS); }
\\ni                        { RET_TOK(_AUTOGEN_, INNI, WC_COMMUT_OPERATOR, REL_CLASS); }
\\not"\in"                         { RET_TOK(ni, INNI, WC_COMMUT_OPERATOR, REL_CLASS); }
\\owns                      { RET_TOK(_AUTOGEN_, INNI, WC_COMMUT_OPERATOR, REL_CLASS); }

 /* parallel */
\\nparallel               { RET_TOK(_AUTOGEN_, PARALL, WC_COMMUT_OPERATOR, REL_CLASS); }
\\parallel                { RET_TOK(_AUTOGEN_, PARALL, WC_COMMUT_OPERATOR, REL_CLASS); }

 /* proportional to */
\\propto                  { RET_TOK(_AUTOGEN_, PROPTO, WC_COMMUT_OPERATOR, REL_CLASS); }
 
 /* ============================
  *  "stack-above" operations
  * ===========================*/
\\stackrel                                                         { return _STACKREL; }
\\buildrel                                                         { return _BUILDREL; }
\\overset                                                           { return _SET_REL; }
\\underset                                                          { return _SET_REL; }
\\xleftarrow                 { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, X_ARROW); }
\\xrightarrow                { RET_TOK(_AUTOGEN_, ARROW, WC_COMMUT_OPERATOR, X_ARROW); }

 /* ============================
  *  skip evironment statements
  * ===========================*/
\\begin\{align\}                                                                      {}
\\end\{align\}                                                                        {}
\\begin\{align\*\}                                                                    {}
\\end\{align\*\}                                                                      {}
\\begin\{alignat\}\{[^}]*\}                                                           {}
\\end\{alignat\}                                                                      {}
\\begin\{alignat\*\}\{[^}]*\}                                                         {}
\\end\{alignat\*\}                                                                    {}
\\begin\{aligned\}                                                                    {}
\\end\{aligned\}                                                                      {}
\\begin\{alignedat\}\{[^}]*\}                                                         {}
\\end\{alignedat\}                                                                    {}
\\begin\{array\}\{[^}]*\}                                                             {}
\\end\{array\}                                                                        {}
\\begin\{eqnarray\}                                                                   {}
\\end\{eqnarray\}                                                                     {}
\\begin\{eqnarray\*\}                                                                 {}
\\end\{eqnarray\*\}                                                                   {}
\\begin\{equation\}                                                                   {}
\\end\{equation\}                                                                     {}
\\begin\{equation\*\}                                                                 {}
\\end\{equation\*\}                                                                   {}
\\begin\{gather\}                                                                     {}
\\end\{gather\}                                                                       {}
\\begin\{gather\*\}                                                                   {}
\\end\{gather\*\}                                                                     {}
\\begin\{gathered\}                                                                   {}
\\end\{gathered\}                                                                     {}
\\begin\{multline\}                                                                   {}
\\end\{multline\}                                                                     {}
\\begin\{multline\*\}                                                                 {}
\\end\{multline\*\}                                                                   {}
\\begin\{split\}                                                                      {}
\\end\{split\}                                                                        {}
\\begin\{subarray\}\{[^}]*\}                                                          {}
\\end\{subarray\}                                                                     {}

 /* ======================
  *  some invisibles left
  * =====================*/
\n                                                                 { return yytext[0]; }
\\[a-zA-Z]+                                    { /* omit undefined control sequence */ }
&                                                   { /* ignore when not in matrix. */ }
"\\!"                                       { /* omit short space, before factorial */ }
"\\;"                                       { /* omit short space, before semicolon */ }
"\\,"                                           { /* omit short space, before comma */ }
[\t ]                                                               { /* omit space */ }
.      { /* lexer_warning_flag = 1; fprintf(stderr, "parser: `%s' esc.\n", yytext); */ }
%%
/* self-defined stack values */
unsigned int ign_stack = 0;
unsigned int mat_stack = 0;
int lexer_warning_flag = 0;

/* =======================================================
 * Symbol ID Space:
 *  0 ... S_N - 1:                         enum symbols
 *  S_N ... S_N + 51:                      a - Z
 *  S_N + 52 ... SYMBOL_ID_VOLUME - 1:     small number
 * ======================================================*/
int ret_var(char *text, struct optr_node **tr, char offset)
{
	int c = (int)(text[0] - offset);
	*tr = optr_alloc(S_N + c, T_VAR, WC_NORMAL_LEAF);

	return VAR;
}

static uint32_t num2symbol_id(uint32_t n)
{
	if (n >= SYMBOL_ID_VOLUME - S_N - 52)
		n = S_bignum;
	else
		n = n + S_N + 52;
	return n;
}

int ret_num(char *text, struct optr_node **tr)
{
	uint32_t n;
	sscanf(text, "%u", &n);

	if (n == 0) {
		*tr = optr_alloc(S_zero, T_ZERO, WC_NORMAL_LEAF);
	} else if (n == 1) {
		*tr = optr_alloc(S_one, T_ONE, WC_NORMAL_LEAF);
	} else {
		n = num2symbol_id(n);
		*tr = optr_alloc(n, T_NUM, WC_NORMAL_LEAF);
	}

	return NUM;
}

int ret_float_num(char *text, struct optr_node **tr)
{
	struct optr_node  *tr_decimal, *tr_intger;
	uint32_t           decimal, intger;

	if (strlen(text) < MAX_FLOAT_STR_LEN) {
		sscanf(text, "%u.%u", &intger, &decimal);

		*tr = optr_alloc(S_float, T_FLOAT, WC_NONCOM_OPERATOR);
		tr_intger = optr_alloc(num2symbol_id(intger), T_NUM, WC_NORMAL_LEAF);
		tr_decimal = optr_alloc(num2symbol_id(decimal), T_NUM, WC_NORMAL_LEAF);

		optr_attach(tr_intger, *tr);
		optr_attach(tr_decimal, *tr);

		return NUM;
	} else {
		fprintf(stderr, "%s is too long to cast into float number (len=%lu).\n",
		        text, strlen(text));

		*tr = optr_alloc(S_zero, T_ZERO, WC_NORMAL_LEAF);
		return NUM;
	}
}

int hack_attach(struct optr_node *tr, char *text, struct optr_node **ret_tr, int ret_tok)
{
	struct optr_node  *tr_a, *tr_b;
	uint32_t a, b, l = strlen(text);

	a = (uint32_t)(text[l - 2] - '0');
	b = (uint32_t)(text[l - 1] - '0');

	tr_a = optr_alloc(num2symbol_id(a), T_NUM, WC_NORMAL_LEAF);
	tr_b = optr_alloc(num2symbol_id(b), T_NUM, WC_NORMAL_LEAF);

	optr_attach(tr_a, tr);
	optr_attach(tr_b, tr);

	*ret_tr = tr;
	return ret_tok;
}
