#include <stddef.h>

#ifndef LEX_PREFIX
#define LEX_PREFIX(_name) yy ## _name
#endif

extern int LEX_PREFIX(lex)(void);
extern FILE *LEX_PREFIX(in);

#undef LEX_PREFIX
