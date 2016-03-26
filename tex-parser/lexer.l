%{
#include <string.h>
#include "y.tab.h"
#include "enum-symbol.h"
#include "enum-token.h"
#include "optr.h"

#define RET_OPTR(_symbol, _token, _wc) \
	yylval.nd = optr_alloc(S_ ## _symbol, T_ ## _token, _wc); \
	return _token;

#define LVAL (&yylval.nd)

int ret_var(char*, struct optr_node **, char);
int ret_num(char*, struct optr_node **);
int ret_float_num(char*, struct optr_node **);

%}

%%
 /* ==================
  *  numbers 
  * ================*/

[0-9]+\.[0-9]*                           { return ret_float_num(yytext, LVAL);         }
[0-9]+                                   { return ret_num(yytext, LVAL);           }

 /* ==================
  *  variables 
  * ================*/

 /* single letter variables */
[a-z]                                    { return ret_var(yytext, LVAL, 'a');      }
[A-Z]                                    { return ret_var(yytext, LVAL, 'A' - 26); }

 /* unicode variables */
α                                          { RET_OPTR(alpha, VAR, WC_NORMAL_LEAF); }
β                                          { RET_OPTR(beta,  VAR, WC_NORMAL_LEAF); }

 /* auto-generated discriminative tokens, where NIL is to be 
  * replaced by specially assigned symbol. */
\\Alpha                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\Beta                                 { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Chi                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Delta                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Epsilon                              { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Eta                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Gamma                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Iota                                 { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\Kappa                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\Lambda                               { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Mu                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Nu                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Omega                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Omicron                              { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\Phi                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Pi                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Psi                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Re                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\Rho                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Sigma                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Tau                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Theta                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Upsilon                              { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\VarLambda                            { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\VarOmega                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Xi                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\Zeta                                 { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\aleph                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\alpha                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\amalg                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\beta                                 { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\beth                                 { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\chi                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\delta                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\ell                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\epsilon                              { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\eta                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\eth                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\gamma                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\imath                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\iota                                 { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\jmath                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\kappa                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\lambda                               { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\mho                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\mu                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\nu                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\omega                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\omicron                              { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\phi                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\psi                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\rho                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\sigma                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\tau                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\theta                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\top                                  { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\upsilon                              { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varDelta                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varGamma                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varPhi                               { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varPi                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\varPsi                               { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varSigma                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varTheta                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varUpsilon                           { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varXi                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varepsilon                           { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varkappa                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varphi                               { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varpi                                { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\varpropto                            { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\varrho                               { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\varsigma                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\vartheta                             { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\wr                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }
\\xi                                   { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); } 
\\zeta                                 { RET_OPTR(_AUTOGEN_, VAR, WC_NORMAL_LEAF); }

 /* ==================
  *  invisible spaces
  * ================*/
\n                                     { return yytext[0]; }
"+"                                    { RET_OPTR(plus, ADD, WC_COMMUT_OPERATOR); }
"±"                                    { RET_OPTR(pm,   ADD, WC_COMMUT_OPERATOR); }
\\pm                                   { RET_OPTR(pm,   ADD, WC_COMMUT_OPERATOR); }
 /* [0-9]               { strcpy(yylval.t, yytext); return NUM; } */
.                                       { }
%%

/*
Symbol ID Space:
	0 ... S_N - 1:                         enum symbols 
	S_N ... S_N + 51:                      a - Z 
	S_N + 52 ... SYMBOL_ID_VOLUME - 1:     small number 
	S_bignum:                              big number
(the first big number is not far from 65303)
*/

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
		return NUM;
	} else if (n == 1) {
		*tr = optr_alloc(S_one, T_ONE, WC_NORMAL_LEAF); 
		return NUM;
	} else {
		n = num2symbol_id(n);
		*tr = optr_alloc(n, T_NUM, WC_NORMAL_LEAF); 
		return NUM;
	}
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
		fprintf(stderr, "%s is too long to cast into float number (len=%d).\n", 
		        text, strlen(text));

		*tr = optr_alloc(S_zero, T_ZERO, WC_NORMAL_LEAF); 
		return NUM;
	}
}
