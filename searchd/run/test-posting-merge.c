#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>

#include "term-index/term-index.h"
#include "postmerge.h"

uint64_t term_posting_current_id_wrap(void *posting)
{
	void *cur_item = term_posting_current(posting);
	doc_id_t doc_id;
	if (cur_item) {
		doc_id = ((struct term_posting_item*)cur_item)->doc_id;
		return (uint64_t)doc_id;
	} else {
		return MAX_POST_ITEM_ID;
	}
}

bool term_posting_jump_wrap(void *posting, uint64_t to_id)
{
	bool succ;

	if (to_id >= UINT_MAX)
		succ = 0;
	else
		succ = term_posting_jump(posting, (doc_id_t)to_id);

	return succ;
}

void posting_on_merge(uint64_t cur_min, struct postmerge_arg* pm_arg, void* extra_args)
{
	uint32_t i;
	for (i = 0; i < pm_arg->n_postings; i++)
		if (pm_arg->curIDs[i] == cur_min)
			printf("mergeID: %lu from posting[%d]\n", cur_min, i);
}

int main(int argc, char *argv[])
{
	int   i, opt;
	void *ti, *posting;
	term_id_t                 term_id;
	struct postmerge_arg      pm_arg;

	char    *index_path = NULL;
	char    *terms[MAX_MERGE_POSTINGS];
	uint32_t n_terms = 0;

	/* initially, set pm_arg.op to undefined */
	postmerge_arg_init(&pm_arg);

	while ((opt = getopt(argc, argv, "hp:t:o:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("test for merge postings.\n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <index path> | -t <term> | -o <op>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp -t 'nick ' -t 'wilde' -o OR\n", argv[0]);
			printf("%s -p ./tmp -t 'give ' -t 'up' -t 'dream' -o AND\n", argv[0]);
			goto free_args;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 't':
			terms[n_terms ++] = strdup(optarg);
			break;

		case 'o':
		{
			if (strcmp(optarg, "AND") == 0)
				pm_arg.op = POSTMERGE_OP_AND;
			else if (strcmp(optarg, "OR") == 0)
				pm_arg.op = POSTMERGE_OP_OR;
			else
				pm_arg.op = POSTMERGE_OP_UNDEF;

			break;
		}

		default:
			printf("bad argument(s). \n");
			goto free_args;
		}
	}

	if (index_path == NULL || n_terms == 0) {
		printf("not enough arguments.\n");
		goto free_args;
	}

	printf("index path: %s\n", index_path);
	printf("terms: ");
	for (i = 0; i < n_terms; i++) {
		printf("`%s'", terms[i]);
		if (i + 1 != n_terms)
			printf(", ");
		else
			printf(".");
	}
	printf("\n");

	ti = term_index_open(index_path, TERM_INDEX_OPEN_EXISTS);
	if (ti == NULL) {
		printf("index not found.\n");
		goto free_args;
	}

	for (i = 0; i < n_terms; i++) {
		term_id = term_lookup(ti, terms[i]);
		if (term_id != 0) {
			printf("term `%s' ID = %u.\n", terms[i], term_id);
			posting = term_index_get_posting(ti, term_id);
			postmerge_arg_add_post(&pm_arg, posting);
		} else {
			printf("term `%s' not found.\n", terms[i]);
			posting = NULL;
		}
	}

	pm_arg.post_start_fun = &term_posting_start;
	pm_arg.post_finish_fun = &term_posting_finish;
	pm_arg.post_jump_fun = &term_posting_jump_wrap;
	pm_arg.post_next_fun = &term_posting_next;
	pm_arg.post_now_id_fun = &term_posting_current_id_wrap;
	pm_arg.post_on_merge = &posting_on_merge;

	if (!posting_merge(&pm_arg, NULL))
		fprintf(stderr, "posting merge operation undefined.\n");

	term_index_close(ti);

free_args:
	for (i = 0; i < n_terms; i++)
		free(terms[i]);

	if (index_path)
		free(index_path);

	return 0;
}
