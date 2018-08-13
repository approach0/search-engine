#include <stdio.h>
#include "math-expr-highlight.h"

int main()
{
	uint64_t mask[3] = {0x50, 0x8, 0x3};
	char *hi = math_oprand_highlight("\\beta + \\lambda + 12 + \\frac \\gamma {n^2 + 1}", mask, 3);
	printf("%s\n", hi);
}
