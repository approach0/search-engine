#include <stdio.h>
#include <stdlib.h>
#include "y.tab.h"

int main()
{
	yyparse();
	printf("hello world!\n");
	return 0;
}
