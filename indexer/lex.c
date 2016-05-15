#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "lex-slice.h"
#include "lex.h"

size_t lex_seek_pos = 0;

static void _strip_math(char *text, size_t bytes)
{
	size_t pad = strlen("[imath]");
	uint32_t i;
	for (i = 0; pad + i + 1 < bytes - pad; i++) {
		text[i] = text[pad + i];
	}

	text[i] = '\0';
}

void _handle_slice(char *text, size_t bytes, enum lex_slice_type type)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.begin = lex_seek_pos - bytes;
	lex_slice.offset = bytes;
	lex_slice.type = type;

	if (type == LEX_SLICE_MATH) {
		_strip_math(text, bytes);
		handle_math(&lex_slice);
	} else if (type == LEX_SLICE_TEXT) {
		handle_text(&lex_slice);
	}
}
