#include "term-index.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	void *posting, *ti;
	char *term;
	struct term_posting_item *pi;

	term_id_t term_id = 117554;
	doc_id_t  doc_id = 5589;

	ti = term_index_open("/home/tk/large-index/term",
	                     TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
		printf("index does not exists.\n");
		goto exit;
	}

	term = term_lookup_r(ti, term_id);
	printf("Lookup: termID=%u (%s) and docID=%u...\n",
	       term_id, term, doc_id);
	free(term);

	posting = term_index_get_posting(ti, term_id);
	if (posting) {
		term_posting_start(posting);
		do {
			pi = term_posting_current(posting);

			if (pi->doc_id == doc_id)
				printf("[docID=%u, tf=%u]\n", pi->doc_id, pi->tf);

		} while (term_posting_next(posting));
		term_posting_finish(posting);
	}

	term_index_close(ti);

exit:
	return 0;
}
