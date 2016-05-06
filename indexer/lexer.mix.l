%option prefix="mix"
%option noyywrap

 /* use yymore option to avoid `yymore_used_but_not_detected' compiler error */
%option yymore

%{
#include <stdio.h>
#include "lex.h"
%}

engword [a-zA-Z]
chinese [\x80-\xf3]
mathsym [\x20-\x7e]

 /* ==================
  *  start conditions
  * =================*/
%x imath

%%
 /* text */
({engword}|{chinese}|[\ ])+             { _handle_slice(yytext, yyleng, LEX_SLICE_TEXT); }

 /* inline math */
"[imath]"  { BEGIN(imath); MORE; /* Append (instead of replace) next tokens to yytext */ }
<imath>"[/imath]"       { BEGIN(INITIAL); _handle_slice(yytext, yyleng, LEX_SLICE_MATH); }
<imath>{mathsym}      { /* In case we encounter "[/imath]", match char by char. */ MORE; }

 /* omitted */
<imath>\n                                                                               {}
<imath>.                                                                                {}
\n                                                                                      {}
.                                                                                       {}
%%
