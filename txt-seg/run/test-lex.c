#include <stdio.h>
#include <string.h>

#include "mhook/mhook.h"

#include "lex.h"
#include "txt-seg.h"
#include "offset-check.h"

#undef NDEBUG
#include <assert.h>

static int my_lex_handler(struct lex_slice *slice)
{
	uint32_t n_bytes = strlen(slice->mb_str);

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH_SEG:
		printf("#%u math: %s <%u, %u>\n",
		       file_offset_check_cnt, slice->mb_str,
		       slice->offset, n_bytes);
		break;

	case LEX_SLICE_TYPE_MIX_SEG:
		printf("#%u mix seg: %s <%u, %u>\n",
		       file_offset_check_cnt, slice->mb_str,
		       slice->offset, n_bytes);
		break;

	case LEX_SLICE_TYPE_ENG_SEG:
		printf("#%u eng seg: %s <%u, %u>\n",
		       file_offset_check_cnt, slice->mb_str,
		       slice->offset, n_bytes);
		break;

	default:
		assert(0);
	}

	file_offset_check_add(slice->offset, n_bytes);
	return 0;
}

typedef int (*test_lex_callback)(FILE *);

static void test(const char *fname, test_lex_callback callbk)
{
	FILE *fh = fopen(fname, "r");

	if (fh == NULL) {
		printf("cannot open `%s'...\n", fname);
		return;
	}

	if (0 != file_offset_check_init(fname))
		goto close;

	/* set global lex handler */
	g_lex_handler = my_lex_handler;

	printf("lexer test [%s]\n", fname);
	callbk(fh);

	file_offset_check_print();
	file_offset_check_free();

close:
	fclose(fh);
	printf("\n");
}

int main(void)
{
	const char test_eng_file_name[] = "test-txt/eng.txt";
	const char test_mix_file_name[] = "test-txt/mix.txt";

	text_segment_init("/home/tk/cppjieba/dict");
	// text_segment_init("/nowhere");

	test(test_eng_file_name, lex_eng_file);
	test(test_mix_file_name, lex_eng_file);
	test(test_mix_file_name, lex_mix_file);

	text_segment_free();
	mhook_print_unfree();
	return 0;
}
