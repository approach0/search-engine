#pragma once
#include <stddef.h>

struct lex_term {
	char  *txt;    /* in multi-bytes */
	size_t begin, offset /* in bytes */;
};
