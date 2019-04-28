#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "mhook/mhook.h"

#include "term-index/term-index.h" /* for position_t */

#include "config.h"
#include "postlist.h"
#include "term-postlist.h"

#define N 20

enum test_option {
	TEST_PLAIN_POSTING,
	TEST_CODEC_POSTING,
};

static struct term_posting_item
*test_gen_item(doc_id_t prev_doc_id)
{
	struct term_posting_item *pi;
	position_t *pos;
	uint32_t i, n, tf;

	n = 1 + rand() % 4; /* number of positions */
	tf = n + rand() % 4; /* term frequency */

	pi = malloc(sizeof (struct term_posting_item));

	pi->doc_id = prev_doc_id + 1 + rand() % 3;
	pi->tf = tf;
	pi->n_occur = n;

	printf("(docID=%u, tf=%u) ", pi->doc_id, n);

	{
		pos = pi->pos_arr;
		pos[0] = rand() % 10;
		for (i = 1; i < n; i++)
			pos[i] = pos[i - 1] + rand() % 100;

		printf("positions: [");
		for (i = 0; i < n; i++)
			printf("%u ", pos[i]);
		printf("]");
	}

	printf("\n");
	return pi;
}


static void test_gen_data(struct postlist *po)
{
	struct   term_posting_item *pi = NULL;
	size_t   fl_sz;
	doc_id_t i, prev = 1;

	for (i = 1; i < N; i++) {
		pi = test_gen_item(prev);

		fl_sz = postlist_write(po, pi, TERM_POSTLIST_ITEM_SZ);

		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);

		prev = pi->doc_id;
		free(pi);
	}

	fl_sz = postlist_write_complete(po);

	printf("Generated %u items ... (linear size: %.2f KB)\n",
		N, N * TERM_POSTLIST_ITEM_SZ / 1024.f);
	postlist_print_info(po);
}

static void test_iterator(struct postlist *po)
{
	struct term_posting_item *pi;
	position_t *pos_arr;
	uint32_t i;
	uint32_t fromID;
	uint64_t jumpID;

	printf("Please input (from, jump_to) IDs:\n");
	(void)scanf("%u, %lu", &fromID, &jumpID);

	postlist_iter_t iter = postlist_iterator(po);
	do {
		pi = (struct term_posting_item*)postlist_iter_cur_item(iter);

		{ /* print current item */
			printf("[docID=%u, tf=%u] ", pi->doc_id, pi->tf);
			pos_arr = pi->pos_arr;
			printf("positions: [");
			for (i = 0; i < pi->n_occur; i++)
				printf("%u ", pos_arr[i]);
			printf("] ");
			printf("\n");
		}

		if (pi->doc_id == fromID) {
			printf("trying to jump to %lu...\n", jumpID);
			int res = postlist_iter_jump32(iter, jumpID);
			printf("jump: %s.\n", res ? "successful" : "failed");
			continue;
		}
	} while (postlist_iter_next(iter));

	postlist_iter_free(iter);
	printf("\n");
}

static void run_testcase(enum test_option opt)
{
	struct postlist *po;
	srand(time(NULL));

	switch (opt) {
	case TEST_PLAIN_POSTING:
		po = term_postlist_create_plain();
		break;
	case TEST_CODEC_POSTING:
		po = term_postlist_create_compressed();
		break;
	default:
		assert(0);
	};

	test_gen_data(po);

	//print_postlist(po);
	test_iterator(po);

	postlist_free(po);
	printf("\n");
}

int main()
{
	printf("{{{ TEST_PLAIN_POSTING }}}\n");
	run_testcase(TEST_PLAIN_POSTING);

	printf("{{{ TEST_CODEC_POSTING }}}\n");
	run_testcase(TEST_CODEC_POSTING);

	mhook_print_unfree();
	return 0;
}
