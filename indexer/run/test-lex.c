#include <stdio.h>
#include <string.h>

#include "indexer.h"

#undef NDEBUG
#include <assert.h>

void lex_slice_handler(struct lex_slice *slice)
{
	uint32_t n_bytes = strlen(slice->mb_str);

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH:
		printf("#%u math: %s <%u, %u>\n",
		       file_offset_check_cnt, slice->mb_str,
		       slice->offset, n_bytes);
		break;

	case LEX_SLICE_TYPE_TEXT:
		printf("#%u text: %s <%u, %u>\n",
		       file_offset_check_cnt, slice->mb_str,
		       slice->offset, n_bytes);
		break;

	case LEX_SLICE_TYPE_ENG_TEXT:
		printf("#%u english: %s <%u, %u>\n",
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
	const char test_file_name[] = "test-doc/1.txt";
	FILE *fh = fopen(test_file_name, "r");

	if (fh == NULL) {
		printf("cannot open document `%s'...\n", test_file_name);
		return 1;
	}

	if (0 != file_offset_check_init(test_file_name))
		goto close;

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
