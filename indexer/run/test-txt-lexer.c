#include <stdlib.h>
#include <ctype.h>
#include "wstring/wstring.h"
#include "txt-seg/txt-seg.h"
#include "term-index/term-index.h"

extern int txtlex();
static void *term_index;

static LIST_IT_CALLBK(handle_chinese_word)
{
	LIST_OBJ(struct term_list_node, p, ln);
	char *term = wstr2mbstr(p->term);

	printf("Chinese word: `%s'\n", term);
	term_index_doc_add(term_index, term);

	LIST_GO_OVER;
}

LIST_DEF_FREE_FUN(list_release, struct term_list_node,
                  ln, free(p));

int handle_chinese(char *txt)
{
	list li = LIST_NULL;

	li = text_segment(txt);
	list_foreach(&li, &handle_chinese_word, NULL);
	list_release(&li);

	return 0;
}

int handle_english(char *term)
{
	for(int i = 0; term[i]; i++)
		term[i] = tolower(term[i]);
	printf("English word:`%s'\n", term);
	term_index_doc_add(term_index, term);
	return 0;
}

int main()
{
	printf("opening dict...\n");
	text_segment_init("../jieba/clone/dict");
	printf("dict opened.\n");

	term_index = term_index_open("./tmp", TERM_INDEX_OPEN_CREATE);
	if (NULL == term_index) {
		printf("cannot create/open term index.\n");
		return 1;
	}

	term_index_doc_begin(term_index);
	txtlex();
	term_index_doc_end(term_index);

	term_index_close(term_index);

	printf("closing dict...\n");
	text_segment_free();
}
