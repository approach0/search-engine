%option prefix="mix"
%option noyywrap

 /* use yymore option to avoid `yymore_used_but_not_detected' compiler error */
%option yymore

%{
#include <stdio.h>
#include "lex.h"
%}

 /* English */
engword [a-zA-Z0-9]

 /* From ASCII 120 to multi-bytes */
chinese [\x80-\xf3]

 /* Almost all characters */
mathsym [\x20-\xf3]

 /* ==================
  *  start conditions
  * =================*/
%x imath

%%
 /* text */
({engword}|{chinese})+                 { lex_handle_mix_text(yytext, yyleng); }

 /* inline math */
"[imath]"            { BEGIN(imath); MORE; /* Append next tokens to yytext */ }
<imath>"[/imath]"          { BEGIN(INITIAL); lex_handle_math(yytext, yyleng); }
<imath>{mathsym}     { /* In case of "[/imath]", match char by char. */ MORE; }

 /* omitted */
<imath>\n                                                                    {}
<imath>.                                                                     {}
\n                                                                           {}
.                                                                            {}
%%
