#include "term-index.h"
#include <stdio.h>

int main()
{
	void *ti = term_index_open("./tmp", TERM_INDEX_OPEN_CREATE);
	if (NULL == ti) {
		printf("cannot create/open term index.\n");
		return 1;
	}

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "hates");
	term_index_doc_add(ti, "cat");
	term_index_doc_end(ti);

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "loves");
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "owner");
	term_index_doc_end(ti);

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "loves");
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "too");
	term_index_doc_end(ti);

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "in");
	term_index_doc_add(ti, "chinese");
	term_index_doc_add(ti, "is"); /* known bug: no upper case */
	term_index_doc_add(ti, "一个字");
	term_index_doc_add(ti, "狗");
	term_index_doc_end(ti);

	term_index_close(ti);
	return 0;
}
