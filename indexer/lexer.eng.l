%option prefix="eng"
%option noyywrap

 /* use yymore option to avoid `yymore_used_but_not_detected' compiler error */
%option yymore

%{
#include <stdio.h>
#include "lex-slice.h"
#include "lex.h"

static void _handle_slice(char*, size_t, enum lex_slice_type);
%}

engword [a-zA-Z]
mathsym [\x20-\x7e]

 /* ==================
  *  start conditions
  * =================*/
%x imath

%%
 /* text */
{engword}+                              { _handle_slice(yytext, yyleng, LEX_SLICE_TEXT); }

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

size_t lex_seek_pos = 0;

static void _handle_slice(char *text, size_t bytes, enum lex_slice_type type)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.begin = lex_seek_pos - bytes;
	lex_slice.offset = bytes;
	lex_slice.type = type;

	if (type == LEX_SLICE_MATH)
		handle_math(&lex_slice);
	else if (type == LEX_SLICE_TEXT)
		handle_text(&lex_slice);
}
