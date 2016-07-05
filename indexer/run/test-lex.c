#include <stdio.h>
#include <string.h>

#include "indexer.h"

#undef NDEBUG
#include <assert.h>

void indexer_handle_slice(struct lex_slice *slice)
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

	default:
		assert(0);
	}

	file_offset_check_add(slice->offset, n_bytes);
}

int main(void)
{
	const char test_file_name[] = "test-doc/1.txt";

	printf("open document `%s'...\n", test_file_name);

	if (0 != file_offset_check_init(test_file_name))
		goto free;

	lex_file(test_file_name);
	file_offset_check_print();

free:
	file_offset_check_free();
	return 0;
}
