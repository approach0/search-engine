#include "tex-parser.h"
#include "yy.h"

int main()
{
	tex_parse("1+2+3", 0);
	return 0;
}
