#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifndef LEX_DEF_HEAD
#define LEX_DEF_HEAD /* begin LEX_DEF_HEAD */

/* lex slice structure */
enum lex_slice_type {
	LEX_SLICE_TYPE_MATH,
	LEX_SLICE_TYPE_ENG_TEXT,
	LEX_SLICE_TYPE_TEXT
};

struct lex_slice {
	char  *mb_str;    /* in multi-bytes */
	uint32_t offset;   /* in bytes */
	enum lex_slice_type type;
};

#define LEX_SLICE_MORE \
	yymore(); \
	lex_bytes_now -= yyleng;
/* append to yytext. */

#define YY_USER_ACTION { lex_bytes_now += yyleng; }

#define YY_USER_INIT { lex_bytes_now = 0; }

#define MORE LEX_SLICE_MORE /* for shorthand */

extern size_t lex_bytes_now;

void lex_handle_math(char*, size_t);
void lex_handle_mix_text(char*, size_t);
void lex_handle_eng_text(char*, size_t);

extern void lex_slice_handler(struct lex_slice *);

void lex_eng_file(FILE*);
void lex_mix_file(FILE*);

#endif /* end LEX_DEF_HEAD */

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
