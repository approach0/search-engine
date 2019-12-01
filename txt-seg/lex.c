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

/* handlers used in .l files */
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

#include <stdlib.h>
#include "config.h"
#include "txt-seg.h"
static LIST_IT_CALLBK(handle_segment)
{
	LIST_OBJ(struct text_seg, seg, ln);
	P_CAST(offset, size_t, pa_extra);

	struct lex_slice lex_slice;
	lex_slice.mb_str = seg->str;
	lex_slice.offset = (uint32_t)(*offset) + seg->offset;
	lex_slice.type = LEX_SLICE_TYPE_MIX_SEG;

	if (g_lex_handler) {
		int res = g_lex_handler(&lex_slice);
		if (res > 0)
			lex_handler_last_err = res;
	}

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(seglist_release, struct text_seg,
                  ln, free(p));

void lex_handle_mix_text(char *text, size_t n_bytes)
{
	size_t offset = lex_bytes_now - n_bytes;

	if (!text_segment_ready()) {
		/* fallback to english handler */
		lex_handle_eng_text(text, n_bytes);
		return;
	}

	list li = LIST_NULL;
	li = text_segment(text); /* text segment */
	list_foreach(&li, &handle_segment, &offset);
	seglist_release(&li);
}
