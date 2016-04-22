%option prefix="txt"
%option noyywrap

 /* use yymore option to avoid `yymore_used_but_not_detected' compiler error */
%option yymore

%{
#include <stdio.h>
#include "lex-term.h"

extern size_t lex_seek_pos;

void handle_text(struct lex_slice *);
void handle_math(struct lex_slice *);

static void _handle_slice(char*, size_t, enum lex_slice_type);

#define LEX_SLICE_MORE \
	yymore(); \
	lex_seek_pos -= txtleng;
/* append to yytext. */

#define YY_USER_ACTION { lex_seek_pos += txtleng; }

#define MORE LEX_SLICE_MORE /* for shorthand */
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
({engword}|{chinese}|[\ ])+            { _handle_slice(txttext, yyleng, LEX_SLICE_TEXT); }

 /* inline math */
"[imath]"                                                          { BEGIN(imath); MORE; }
<imath>"[/imath]"      { BEGIN(INITIAL); _handle_slice(txttext, yyleng, LEX_SLICE_MATH); }
<imath>{mathsym}      { MORE; /* In case we encounter "[/imath]", match char by char. */ }

 /* omitted */
<imath>.                                                                                {}
<imath>\n                                                                               {}

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
