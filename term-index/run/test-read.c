#include "term-index.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char* utf8cpy(char* dst, const char* src)
{
	size_t sizeSrc = strlen(src);
	memcpy(dst, src, sizeSrc);
	dst[sizeSrc] = '\0';
	return dst;
}

int main()
{
	char term[2048];
	void *posting;
	struct term_posting_item *pi;
	void *ti = term_index_open("./tmp", TERM_INDEX_OPEN_EXISTS);
	uint32_t termN;
	uint32_t docN;
	uint32_t avgDocLen;
	term_id_t i;
	doc_id_t j;

	if (ti == NULL) {
		printf("index does not exists.\n");
		return 1;
	}

	termN = term_index_get_termN(ti);
	docN = term_index_get_docN(ti);
	avgDocLen = term_index_get_avgDocLen(ti);

	printf("Index summary:\n");
	printf("termN: %u\n", termN);
	printf("docN: %u\n", docN);
	printf("avgDocLen: %u\n", avgDocLen);

	for (i = 1; i <= termN; i++) {
		utf8cpy(term, term_lookup_r(ti, i));
		printf("term#%u=", term_lookup(ti, term));
		printf("'%s' ", term);
		printf("(df=%u): ", term_index_get_df(ti, i));

		posting = term_index_get_posting(ti, i);
		if (posting) {
			term_posting_start(posting);
			while ((pi = term_posting_current(posting)) != NULL) {
				printf("[docID=%u, tf=%u] ", pi->doc_id, pi->tf);
				term_posting_next(posting);
			}
			term_posting_finish(posting);
			printf("\n");
		}
	}

	for (j = 1; j <= docN; j++) {
		printf("doc#%u length = %u\n", j, term_index_get_docLen(ti, j));
	}

	term_index_close(ti);
	return 0;
}
