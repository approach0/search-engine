#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "txt-seg/txt-seg.h"
#include "indexer.h"

#undef NDEBUG
#include <assert.h>

LIST_DEF_FREE_FUN(list_release, struct text_seg,
                  ln, free(p));

static LIST_IT_CALLBK(add_check_node)
{
	LIST_OBJ(struct text_seg, seg, ln);
	P_CAST(slice_offset, uint32_t, pa_extra);

	seg->offset += *slice_offset;

	printf("#%u text: %s <%u, %u>\n", file_offset_check_cnt,
	       seg->str, seg->offset, seg->n_bytes);

	file_offset_check_add(seg->offset, seg->n_bytes);

	LIST_GO_OVER;
}

void lex_slice_handler(struct lex_slice *slice)
{
	list li = LIST_NULL;
	printf("input slice: [%s]\n", slice->mb_str);

	switch (slice->type) {
	case LEX_SLICE_TYPE_MATH:
		printf("#%u math: %s <%u, %lu>\n",
		       file_offset_check_cnt, slice->mb_str,
		       slice->offset, strlen(slice->mb_str));

		file_offset_check_add(slice->offset, strlen(slice->mb_str));
		break;

	case LEX_SLICE_TYPE_TEXT:
		li = text_segment(slice->mb_str);
		list_foreach(&li, &add_check_node, &slice->offset);
		break;

	default:
		assert(0);
	}

	list_release(&li);
}

int main(void)
{
	const char test_file_name[] = "test-doc/1.txt";

	printf("open dictionary ...\n");
	text_segment_init("");

	printf("open document `%s'...\n", test_file_name);
	if (0 != file_offset_check_init(test_file_name))
		goto free;

	lex_file_mix(test_file_name);
	file_offset_check_print();

free:
	file_offset_check_free();

	printf("closing dict...\n");
	text_segment_free();
	return 0;
}
