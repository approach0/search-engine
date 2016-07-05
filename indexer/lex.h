#include <stddef.h>
#include "indexer.h"

extern size_t lex_bytes_now;

void lex_handle_math(char*, size_t);
void lex_handle_text(char*, size_t);

extern void indexer_handle_slice(struct lex_slice *);

#define LEX_SLICE_MORE \
	yymore(); \
	lex_bytes_now -= yyleng;
/* append to yytext. */

#define YY_USER_ACTION { lex_bytes_now += yyleng; }

#define YY_USER_INIT { lex_bytes_now = 0; }

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
