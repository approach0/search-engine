#include <string.h>
#include <stddef.h>
#include <stdio.h>

size_t lex_bytes_now = 0;

/* mix (Chinese and English) lexer */
#undef   LEX_PREFIX
#define  LEX_PREFIX(_name) mix ## _name
#include "lex.h"

void lex_file_mix(const char* path)
{
	FILE *fh = fopen(path, "r");
	if (fh) {
		LEX_PREFIX(in) = fh;
		DO_LEX;
		fclose(fh);
	} else {
		fprintf(stderr, "cannot open `%s'.\n", path);
	}
}

/* English-only lexer (faster) */
#undef   LEX_PREFIX
#define  LEX_PREFIX(_name) eng ## _name
#include "lex.h"

void lex_file_eng(const char* path)
{
	FILE *fh = fopen(path, "r");
	if (fh) {
		LEX_PREFIX(in) = fh;
		DO_LEX;
		fclose(fh);
	} else {
		fprintf(stderr, "cannot open `%s'.\n", path);
	}
}

void lex_handle_text(char *text, size_t n_bytes)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.offset = lex_bytes_now - n_bytes;
	lex_slice.type = LEX_SLICE_TYPE_TEXT;

	lex_slice_handler(&lex_slice);
}

void lex_handle_eng_text(char *text, size_t n_bytes)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.offset = lex_bytes_now - n_bytes;
	lex_slice.type = LEX_SLICE_TYPE_ENG_TEXT;

	lex_slice_handler(&lex_slice);
}

void lex_handle_math(char *text, size_t n_bytes)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.offset = lex_bytes_now - n_bytes;
	lex_slice.type = LEX_SLICE_TYPE_MATH;

	lex_slice_handler(&lex_slice);
}
