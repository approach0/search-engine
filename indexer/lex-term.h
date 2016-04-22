#pragma once
#include <stddef.h>

enum lex_slice_type {
	LEX_SLICE_TEXT,
	LEX_SLICE_MATH
};

struct lex_slice {
	char  *mb_str;    /* in multi-bytes */
	size_t begin, offset /* in bytes */;
	enum lex_slice_type type;
};
