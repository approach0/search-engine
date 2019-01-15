%option prefix="eng"
%option noyywrap

 /* use yymore option to avoid `yymore_used_but_not_detected' compiler error */
%option yymore

%{
#include <stdio.h>
#include "lex.h"
%}

engword [a-zA-Z]
mathsym [\x20-\xf3]

 /* ==================
  *  start conditions
  * =================*/
%x imath

%%
 /* text */
{engword}+                             { lex_handle_eng_text(yytext, yyleng); }

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
