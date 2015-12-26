%{
#include "y.tab.h"
#include "enum-symbol.h"
#include "enum-token.h"
%}

%%
 /* invisible and space */

\n                                      { return yytext[0]; }
"+"                                     { return yytext[0]; }
[0-9]                                   { return NUM; }
.                                       { }
%%
