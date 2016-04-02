#include <stdio.h>
#include <stdlib.h>

#define OPTR_HASH_DEBUG
#include "optr.h"

int main()
{
	symbol_id_t hash;
	hash = 0xcdab;
	printf("%s\n", optr_hash_str(hash));

	hash = 435;
	printf("%s\n", optr_hash_str(hash));

	tex_parse("ab+cd = ab+cd", 0);
	return 0;
}
