#include "mhook/mhook.h"
#include "term-index.h"
#include <stdio.h>

#define ADD(_ti, _term) \
	term_index_doc_add(_ti, _term);

int main()
{
	void *ti = term_index_open("./tmp", TERM_INDEX_OPEN_CREATE);
	doc_id_t doc_id;

	if (NULL == ti) {
		printf("cannot create/open term index.\n");
		return 1;
	}

	term_index_doc_begin(ti);
	ADD(ti, "dog");
	ADD(ti, "hates");
	ADD(ti, "cat");
	doc_id = term_index_doc_end(ti);
	printf("document#%u added.\n", doc_id);

	term_index_doc_begin(ti);
	ADD(ti, "dog");
	ADD(ti, "loves");
	ADD(ti, "dog");
	ADD(ti, "owner");
	doc_id = term_index_doc_end(ti);
	printf("document#%u added.\n", doc_id);

	term_index_doc_begin(ti);
	ADD(ti, "dog");
	ADD(ti, "loves");
	ADD(ti, "dog");
	ADD(ti, "too");
	doc_id = term_index_doc_end(ti);
	printf("document#%u added.\n", doc_id);

	term_index_doc_begin(ti);
	ADD(ti, "dog");
	ADD(ti, "in");
	ADD(ti, "chinese"); /* known bug: upper case causes core dumped */
	ADD(ti, "is");
	ADD(ti, "一个字");
	ADD(ti, "狗");
	doc_id = term_index_doc_end(ti);
	printf("document#%u added.\n", doc_id);

	printf("closing term index...\n");
	term_index_close(ti);

	mhook_print_unfree();
	return 0;
}
