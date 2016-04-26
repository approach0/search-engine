#include <stddef.h>

extern size_t lex_seek_pos;

void handle_text(struct lex_slice *);
void handle_math(struct lex_slice *);

#define LEX_SLICE_MORE \
	yymore(); \
	lex_seek_pos -= yyleng;
/* append to yytext. */

#define YY_USER_ACTION { lex_seek_pos += yyleng; }

#define YY_USER_INIT { lex_seek_pos = 0; }

#define MORE LEX_SLICE_MORE /* for shorthand */

/* prefix-dependent declarations */
#ifndef LEX_PREFIX
#define LEX_PREFIX(_name) yy ## _name
#endif

extern int   LEX_PREFIX(lex)(void);
extern FILE *LEX_PREFIX(in);
extern int   LEX_PREFIX(lex_destroy)(void);

/* define a DO_LEX macro to make sure yylex_destory()
 * is following each yylex(), so that YY_USER_INIT will
 * be called before each yylex() is performed. */
#define DO_LEX \
	LEX_PREFIX(lex)(); \
	LEX_PREFIX(lex_destroy)()
