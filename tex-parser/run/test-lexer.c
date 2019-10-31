#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "head.h"
#include "y.tab.h"

char *filter(char *str)
{
	if (0 == strcmp(str, "\n")) {
		return "\\n";
	} else {
		return str;
	}
}

int main()
{
	char input[] = "a+b=c\n\\frac 1 \\pi";
	FILE *test_fh = fmemopen(input, strlen(input), "r");
	if (test_fh == NULL) {
		printf("cannot open test file.\n");
		return 1;
	}

	int lexer_tok, grammar_tok;
	yyin = test_fh;

	while ((lexer_tok = yylex())) {
		struct optr_node* nd = yylval.nd;

		printf("%d: %s ", lexer_tok, filter(yytext));
		if (nd) {
			grammar_tok = nd->token_id;
			char *tok_name = trans_token(grammar_tok);
			char *symbol = trans_symbol(nd->symbol_id);
			printf("(OPT node: %s, %s)", tok_name, symbol);

			optr_release(nd);
			yylval.nd = NULL;
		}
		printf("\n");
	}
	yylex_destroy();

	fclose(test_fh);

	mhook_print_unfree();
	return 0;
}
