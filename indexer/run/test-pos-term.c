#include <stdio.h>
#include "indexer.h"

void handle_math(struct lex_slice *slice)
{
	printf("math: %s <%u,%u>\n", slice->mb_str,
	       slice->begin, slice->offset);
}

static LIST_IT_CALLBK(check_pos)
{
	LIST_OBJ(struct splited_term, st, ln);

	printf("#%u term: `%s' pos(%u[%u])\n", file_pos_check_cnt,
	       st->term, st->doc_pos, st->n_bytes);
	file_pos_check_add(st->doc_pos, st->n_bytes);

	LIST_GO_OVER;
}

extern void handle_text(struct lex_slice *slice)
{
	list splited_terms = LIST_NULL;
	printf("slice<%u,%u>: %s\n",
	       slice->begin, slice->offset, slice->mb_str);

	/* split slice into terms */
	split_into_terms(slice, &splited_terms);
	list_foreach(&splited_terms, &check_pos, NULL);
	release_splited_terms(&splited_terms);
}

int main(void)
{
	const char test_file_name[] = "test-doc/1.txt";

	printf("opening dict...\n");
	text_segment_init("../jieba/fork/dict");

	if (0 != file_pos_check_init(test_file_name))
		goto free;

	lex_txt_file(test_file_name);
	file_pos_check_print();

free:
	file_pos_check_free();
	
	printf("closing dict...\n");
	text_segment_free();
	return 0;
}
