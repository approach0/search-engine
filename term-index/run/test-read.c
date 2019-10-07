#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>

#undef N_DEBUG
#include <assert.h>

#include "mhook/mhook.h"
#include "common/common.h"

#include "term-index/config.h"
#include "term-index.h"

int opt, opt_any = 0;
bool opt_summary = 0, opt_terms = 0, opt_postings = 0,
	 opt_document = 0, opt_posting_termpos = 0, opt_all = 0;

static void print_pos_arr(position_t *pos_arr, uint n)
{
	printf(", pos=");
	for (uint k = 0; k < n; k++)
		printf("%d%c", pos_arr[k], (k == n - 1) ? '.' : ',');
}

static void print_inmemo(invlist_iter_t iter)
{
	struct term_posting_item item;
	iterator_set_bufkey_to_32(iter);
	printf("in-memo: ");

	do {
		uint64_t key = invlist_iter_curkey(iter);
		size_t rd_sz = invlist_iter_read(iter, &item);
		(void)rd_sz;
		assert(key == item.doc_id);

		printf("[docID=%u, tf=%u", item.doc_id, item.tf);
		if (opt_all || opt_posting_termpos) {
			/* read term positions in this document */
			print_pos_arr(item.pos_arr, item.n_occur);
		}
		printf("]");
	} while (invlist_iter_next(iter));

	invlist_iter_free(iter);
	printf("\n");
}

static void print_ondisk(void *reader)
{
	printf("on-disk: ");
	term_posting_start(reader);

	do {
		/* test both "get_cur_item" functions */
		struct term_posting_item pi;
		uint64_t doc_id = term_posting_cur(reader);
		term_posting_read(reader, &pi);

		assert(doc_id == pi.doc_id);

		printf("[docID=%u, tf=%u", pi.doc_id, pi.tf);

		if (opt_all || opt_posting_termpos) {
			/* read term positions in this document */
			print_pos_arr(pi.pos_arr, pi.n_occur);
		}
		printf("]");

	} while (term_posting_next(reader));

	term_posting_finish(reader);
	printf("\n");
}

static void print_help(char *argv[])
{
	printf("DESCRIPTION:\n");
	printf("dump term index.\n");
	printf("\n");
	printf("USAGE:\n");
	printf("%s -h |"
	       " -p (index path) |"
	       " -c <cache size (KB)> |"
	       " -s (summary) |"
	       " -t (unique terms) |"
	       " -l (posting lists) |"
	       " -d (all document) |"
	       " -o (term positions) |"
	       " -a (dump all) |"
	       " -g <df> "
	       "(list terms whose df is greater than <df>) |"
	       " -i <term_id> "
	       "(list only posting list of <term_id>)"
	       "\n", argv[0]);
}

int main(int argc, char* argv[])
{
	void *ti;
	uint32_t termN, docN, avgDocLen;
	term_id_t i;
	doc_id_t j;
	char *term;
	char *index_path = NULL;
	uint32_t df, df_valve = 0;
	uint32_t term_id = 0;
	size_t cache_limit = 0;

	while ((opt = getopt(argc, argv, "hstldoap:g:i:c:")) != -1) {
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

		case 'c':
			sscanf(optarg, "%lu", &cache_limit);
			cache_limit = cache_limit KB;
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

	term_index_load(ti, cache_limit);

	if (term_id != 0)
		printf("only list term ID: %u\n", term_id);

	termN = term_index_get_termN(ti);
	docN = term_index_get_docN(ti);
	avgDocLen = term_index_get_avgDocLen(ti);

	if (opt_all || opt_summary) {
		printf("\n Index summary:\n");
		printf("termN: %u\n", termN);
		printf("docN: %u\n", docN);
		printf("avgDocLen: %u\n", avgDocLen);
	}

	printf("\n Term printing:\n");
	for (i = 1; i <= termN; i++) {
		if (term_id != 0 && i != term_id)
			continue; /* skip this term */

		if (opt_all || opt_terms) {
			df = term_index_get_df(ti, i);
			if (df > df_valve) {
				term = term_lookup_r(ti, i);
				printf("term#%u: ", term_lookup(ti, term));
				printf("`%s' ", term);
				printf("(df=%u). ", df);
				free(term);
			}
		}

		if (opt_all || opt_postings) {
			struct term_invlist_entry_reader entry_reader;
			entry_reader = term_index_lookup(ti, i);
			printf("(check df=%u) ", entry_reader.df);
			if (entry_reader.inmemo_reader)
				print_inmemo(entry_reader.inmemo_reader);
			if (entry_reader.ondisk_reader)
				print_ondisk(entry_reader.ondisk_reader);
		}
	}

	if (opt_all || opt_document) {
		printf("\n Document printing:\n");
		for (j = 1; j <= docN; j++) {
			printf("doc#%u length = %u\n", j, term_index_get_docLen(ti, j));
		}
	}

	term_index_close(ti);

exit:
	free(index_path);
	mhook_print_unfree();
	return 0;
}
