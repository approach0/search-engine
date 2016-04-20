#include "term-index.h"
#include <stdio.h>

int main()
{
	void *ti = term_index_open("./tmp", TERM_INDEX_OPEN_CREATE);
	doc_id_t doc_id;

	if (NULL == ti) {
		printf("cannot create/open term index.\n");
		return 1;
	}

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "hates");
	term_index_doc_add(ti, "cat");
	doc_id = term_index_doc_end(ti);
	printf("doc#%u added.\n", doc_id);

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "loves");
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "owner");
	doc_id = term_index_doc_end(ti);
	printf("doc#%u added.\n", doc_id);

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "loves");
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "too");
	doc_id = term_index_doc_end(ti);
	printf("doc#%u added.\n", doc_id);

	term_index_doc_begin(ti);
	term_index_doc_add(ti, "dog");
	term_index_doc_add(ti, "in");
	term_index_doc_add(ti, "chinese"); /* known bug: upper case causes core dumped */
	term_index_doc_add(ti, "is");
	term_index_doc_add(ti, "一个字");
	term_index_doc_add(ti, "狗");
	doc_id = term_index_doc_end(ti);
	printf("doc#%u added.\n", doc_id);

	printf("closing term index...\n");
	term_index_close(ti);
	return 0;
}
