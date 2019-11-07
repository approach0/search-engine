#include <assert.h>
#include <string.h>
#include "txt2snippet.h"
#include "txt-seg/lex.h"

static uint32_t *positions;
static uint32_t  positions_cur, positions_len;
static uint32_t  cur_lex_pos;
static list      hi_list;

static int handle_slice(struct lex_slice*);

list txt2snippet(const char *txt, size_t txt_sz, uint32_t *pos, uint32_t n)
{
	FILE *txt_fh;

	/* highlighter arguments */
	positions = pos;
	positions_cur = 0;
	positions_len = n;
	cur_lex_pos = 0;
	LIST_CONS(hi_list);

	/* register lex handler  */
	g_lex_handler = handle_slice;

	/* invoke lexer */
	txt_fh = fmemopen((void *)txt, txt_sz, "r");
	lex_eng_file(txt_fh);

	/* print snippet */
	snippet_read_file(txt_fh, &hi_list);

	/* close file handler */
	fclose(txt_fh);

	return hi_list;
}

static void
add_highlight_seg(char *mb_str, uint32_t offset, size_t sz, void *arg)
{
	/* early termination due to exhaustive positions */
	if (positions_cur == positions_len)
		return;

	// printf("%s@%u ?= %u\n", mb_str, cur_lex_pos, positions[positions_cur]);

	/* if this segment is any of our interested positions */
	if (cur_lex_pos == positions[positions_cur]) {
		snippet_push_highlight(&hi_list, mb_str, offset, sz);
		positions_cur ++;
	}

	cur_lex_pos ++;
}

static int handle_slice(struct lex_slice *slice)
{
	size_t str_sz = strlen(slice->mb_str);

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH_SEG:
		/* this is a math segment */

		add_highlight_seg(slice->mb_str, slice->offset, str_sz, NULL);
		break;

	case LEX_SLICE_TYPE_MIX_SEG:
		fprintf(stderr, "Not implemented for mix-seg.\n");
		assert(0);

		break;

	case LEX_SLICE_TYPE_ENG_SEG:
		/* this is a pure English segment */

		add_highlight_seg(slice->mb_str, slice->offset, str_sz, NULL);
		break;

	default:
		fprintf(stderr, "Unexpected seg.\n");
		assert(0);
	}

	return 0;
}
