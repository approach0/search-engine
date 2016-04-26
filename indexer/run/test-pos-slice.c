#include <stdio.h>
#include "indexer.h"

void handle_math(struct lex_slice *slice)
{
	printf("#%u math: %s <%u,%u>\n", file_pos_check_cnt, slice->mb_str,
	       slice->begin, slice->offset);
	file_pos_check_add(slice->begin, slice->offset);
}

void handle_text(struct lex_slice *slice)
{
	printf("#%u text: %s <%u,%u>\n", file_pos_check_cnt, slice->mb_str,
	       slice->begin, slice->offset);
	file_pos_check_add(slice->begin, slice->offset);
}

int main(void)
{
	const char test_file_name[] = "test-doc/1.txt";

	if (0 != file_pos_check_init(test_file_name))
		goto free;

	lex_txt_file(test_file_name);
	file_pos_check_print();

free:
	file_pos_check_free();
	return 0;
}
