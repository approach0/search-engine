#include <string.h>
#include "lex.h"

lex_handle_callbk g_lex_handler = NULL;
static int lex_handler_last_err = 0;

size_t lex_bytes_now = 0;

/* mixed text (Chinese and English) lexer */
#undef   LEX_PREFIX
#define  LEX_PREFIX(_name) mix ## _name
#include "lex.h"

int lex_mix_file(FILE *fh)
{
	lex_handler_last_err = 0;
	fseek(fh, 0, SEEK_SET);
	LEX_PREFIX(in) = fh;
	DO_LEX;
	return lex_handler_last_err;
}

/* English-only lexer (faster) */
#undef   LEX_PREFIX
#define  LEX_PREFIX(_name) eng ## _name
#include "lex.h"

int lex_eng_file(FILE *fh)
{
	lex_handler_last_err = 0;
	fseek(fh, 0, SEEK_SET);
	LEX_PREFIX(in) = fh;
	DO_LEX;
	return lex_handler_last_err;
}

void lex_handle_mix_text(char *text, size_t n_bytes)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.offset = lex_bytes_now - n_bytes;
	lex_slice.type = LEX_SLICE_TYPE_MIX_SEG;

	if (g_lex_handler) {
		int res = g_lex_handler(&lex_slice);
		if (res > 0)
			lex_handler_last_err = res;
	}
}

void lex_handle_eng_text(char *text, size_t n_bytes)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.offset = lex_bytes_now - n_bytes;
	lex_slice.type = LEX_SLICE_TYPE_ENG_SEG;

	if (g_lex_handler) {
		int res = g_lex_handler(&lex_slice);
		if (res > 0)
			lex_handler_last_err = res;
	}
}

void lex_handle_math(char *text, size_t n_bytes)
{
	struct lex_slice lex_slice;
	lex_slice.mb_str = text;
	lex_slice.offset = lex_bytes_now - n_bytes;
	lex_slice.type = LEX_SLICE_TYPE_MATH_SEG;

	if (g_lex_handler) {
		int res = g_lex_handler(&lex_slice);
		if (res > 0)
			lex_handler_last_err = res;
	}
}
