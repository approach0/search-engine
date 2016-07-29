#include <stdio.h>
#include <string.h>

#include "lex.h"
#include "offset-check.h"

#undef NDEBUG
#include <assert.h>

static void my_lex_handler(struct lex_slice *slice)
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
}

int main(void)
{
	const char test_file_name[] = "test-txt/eng.txt";
	FILE *fh = fopen(test_file_name, "r");

	if (fh == NULL) {
		printf("cannot open `%s'...\n", test_file_name);
		return 1;
	}

	if (0 != file_offset_check_init(test_file_name))
		goto close;

	/* set global lex handler */
	g_lex_handler = my_lex_handler;

	printf("testing English lexer...\n");
	lex_eng_file(fh);

	file_offset_check_print();
	file_offset_check_free();

	if (0 != file_offset_check_init(test_file_name))
		goto close;

	printf("testing mix lexer...\n");
	lex_mix_file(fh);

	file_offset_check_print();
	file_offset_check_free();

close:
	fclose(fh);
	return 0;
}
