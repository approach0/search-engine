#include "term-index.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

#undef N_DEBUG
#include <assert.h>

static void print_help(char *argv[])
{
	printf("DESCRIPTION:\n");
	printf("dump term index.\n");
	printf("\n");
	printf("USAGE:\n");
	printf("%s -h |"
	       " -s (summary) |"
	       " -t (unique terms) |"
	       " -l (posting lists) |"
	       " -d (all document) |"
	       " -o (term positions) |"
	       " -a (dump all) |"
	       " -p (index path) |"
	       " -g <df> "
	       "(list terms whose df is greater than <df>) |"
	       " -i <term_id> "
	       "(list only posting list of <term_id>)"
	       "\n", argv[0]);
}

int main(int argc, char* argv[])
{
	void *posting, *ti;
	struct term_posting_item *pi, *pip;
	uint32_t k, termN, docN, avgDocLen;
	position_t *pos_arr;
	term_id_t i;
	doc_id_t j;
	char *term;
	char *index_path = NULL;
	uint32_t df, df_valve = 0;
	uint32_t term_id = 0;

	int opt, opt_any = 0;
	bool opt_summary = 0, opt_terms = 0, opt_postings = 0,
	     opt_document = 0, opt_posting_termpos = 0, opt_all = 0;
	while ((opt = getopt(argc, argv, "hstldoap:g:i:")) != -1) {
		opt_any = 1;
		switch (opt) {
		case 'h':
			print_help(argv);
			goto exit;

		case 's':
			opt_summary = 1;
			break;

		case 't':
			opt_terms = 1;
			break;

		case 'l':
			opt_postings = 1;
			break;

		case 'd':
			opt_document = 1;
			break;

		case 'o':
			opt_posting_termpos = 1;
			break;

		case 'a':
			opt_all = 1;
			break;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 'g':
			sscanf(optarg, "%d", &df_valve);
			printf("df valve: %u\n", df_valve);
			break;

		case 'i':
			sscanf(optarg, "%d", &term_id);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (!opt_any) {
		print_help(argv);
		goto exit;
	}

	if (index_path == NULL) {
		printf("no index path specified.\n");
		goto exit;
	} else {
		printf("open index: %s\n", index_path);
	}

	ti = term_index_open(index_path, TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
		printf("index does not exists.\n");
		goto exit;
	}

	if (term_id != 0)
		printf("only list term ID: %u\n", term_id);

	termN = term_index_get_termN(ti);
	docN = term_index_get_docN(ti);
	avgDocLen = term_index_get_avgDocLen(ti);

	if (opt_all || opt_summary) {
		printf("Index summary:\n");
		printf("termN: %u\n", termN);
		printf("docN: %u\n", docN);
		printf("avgDocLen: %u\n", avgDocLen);
	}

	for (i = 1; i <= termN; i++) {
		if (term_id != 0 && i != term_id)
			continue; /* skip this term */

		if (opt_all || opt_terms) {
			df = term_index_get_df(ti, i);
			if (df > df_valve) {
				term = term_lookup_r(ti, i);
				printf("term#%u=", term_lookup(ti, term));
				printf("`%s' ", term);
				printf("(df=%u): ", term_index_get_df(ti, i));
				free(term);
			}
		}

		if (opt_all || opt_postings) {
			posting = term_index_get_posting(ti, i);
			if (posting) {
				term_posting_start(posting);
				do {
					/* test both "get_cur_item" functions */
					pi = term_posting_cur_item(posting);
					pip = term_posting_cur_item_with_pos(posting);

					assert(pi->doc_id == pip->doc_id);
					assert(pi->tf == pip->tf);

					printf("[docID=%u, tf=%u", pi->doc_id, pi->tf);

					if (opt_all || opt_posting_termpos) {
						/* read term positions in this document */
						printf(", pos=");
						pos_arr = TERM_POSTING_ITEM_POSITIONS(pip);
						for (k = 0; k < pi->tf; k++)
							printf("%d%c", pos_arr[k],
							       (k == pi->tf - 1) ? '.' : ',');
						printf("]");
					} else {
						printf("]");
					}

				} while (term_posting_next(posting));
				term_posting_finish(posting);
				printf("\n");
			}
		}
	}

	if (opt_all || opt_document) {
		for (j = 1; j <= docN; j++) {
			printf("doc#%u length = %u\n", j, term_index_get_docLen(ti, j));
		}
	}

	term_index_close(ti);

exit:
	if (index_path)
		free(index_path);

	return 0;
}
