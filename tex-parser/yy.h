#include <stddef.h>
extern int yyparse();
extern int yyerror(const char *);
extern int yylex();
extern int yylex_destroy();

extern FILE *yyin;
extern char *yytext;

struct yy_buffer_state;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
typedef size_t yy_size_t;

YY_BUFFER_STATE yy_scan_buffer(char *, yy_size_t);
void yy_delete_buffer(YY_BUFFER_STATE);

YY_BUFFER_STATE yy_scan_string(const char *);
