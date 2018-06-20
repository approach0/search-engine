#include "mhook/mhook.h"
#include "tex-parser/vt100-color.h"
#include "indexer/config.h" // for MAX_CORPUS_FILE_SZ
#include "indices.h"
#include "config.h"

#undef N_DEBUG
#include <assert.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

int main()
{
	struct indices indices;
	const char index_path[] = "../indexer/tmp";

	uint32_t i, termN, termID;
	char *term;
	const uint32_t max_terms_listed = 100;
	void *posting;
	struct term_posting_item *pip;
	position_t *pos_arr;

	if(indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	termN = term_index_get_termN(indices.ti);
	printf("%u term(s) in indices, listed are top %u:\n", termN,
	       max_terms_listed);

	for (i = 1; i <= MIN(max_terms_listed, termN); i++) {
		term = term_lookup_r(indices.ti, i);
		printf("#%u: `%s' ", i, term);
		free(term);
	}
	printf("\n");

	printf("Please input termID:\n");
	scanf("%u", &termID);

	posting = term_index_get_posting(indices.ti, termID);
	if (posting) {
		term_posting_start(posting);

		do {
			pip = term_posting_cur_item_with_pos(posting);
			pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);

			printf("[docID=%u, tf=%u", pip->doc_id, pip->tf);

			printf(", pos=");
			for (i = 0; i < pip->tf; i++) {
				printf("%d%c", pos_arr[i], (i == pip->tf - 1) ? '.' : ',');
			}

			printf("]");

		} while (term_posting_next(posting));

		term_posting_finish(posting);
		printf("\n");
	}

close:
	indices_close(&indices);

	mhook_print_unfree();
	return 0;
}
