%option prefix="txt"
%option noyywrap

%{
#include <stdio.h>
#include "lex-term.h"

int handle_chinese(struct lex_term *);
int handle_english(struct lex_term *);

enum handle_term_type {
	HANDLE_CHINESE,
	HANDLE_ENGLISH
};

extern size_t lex_seek_pos;
static void   handle_term(char*, enum handle_term_type);

#define YY_USER_ACTION { lex_seek_pos += txtleng; }
%}

engword [a-zA-Z]
chinese [\x80-\xf3]

%%
{engword}+                             { handle_term(txttext, HANDLE_ENGLISH); }
{chinese}+                             { handle_term(txttext, HANDLE_CHINESE); }
"[imath]".*"[/imath]"                  { printf("inline math: `%s'\n", txttext); }
\n                                     {}
.                                      {}
%%

size_t lex_seek_pos = 0;

static void handle_term(char *term, enum handle_term_type type)
{
	struct lex_term lex_term;
	size_t len = strlen(term); /* in bytes */
	
	lex_term.txt = term;
	lex_term.begin = lex_seek_pos - len;
	lex_term.offset = len;

	if (type == HANDLE_ENGLISH)
		handle_english(&lex_term);
	else
		handle_chinese(&lex_term);
}
