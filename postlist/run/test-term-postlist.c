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
*test_gen_item(const doc_id_t docID)
{
	struct term_posting_item *pi;
	position_t *pos;
	uint32_t i, n;

	n = 1 + rand() % 4; /* number of positions */

	pi = malloc(TERM_POSTLIST_ITEM_SZ);

	pi->doc_id = docID;
	pi->tf = n;

	printf("(docID=%u, tf=%u) ", docID, n);

	{
		pos = (position_t *)(pi + 1);
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
	struct   term_posting_item *pi;
	size_t   fl_sz;
	doc_id_t i;

	for (i = 1; i < N; i++) {
		pi = test_gen_item(i);

		// printf("writing %lu bytes...\n", TERM_POSTLIST_ITEM_SZ);
		fl_sz = postlist_write(po, pi, TERM_POSTLIST_ITEM_SZ);

		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);

		free(pi);
	}

	fl_sz = postlist_write_complete(po);
	printf("flush %lu bytes.\n", fl_sz);
}

static void test_iterator(struct postlist *po)
{
	struct term_posting_item *pi;
	position_t *pos_arr;
	uint32_t i;
	uint32_t fromID;
	uint64_t jumpID;
	int      jumped = 0;

	printf("Please input (from, jump_to) IDs:\n");
	scanf("%u, %lu", &fromID, &jumpID);

	postlist_start(po);

	do {
print_current:
		pi = postlist_cur_item(po);

		printf("[docID=%u, tf=%u] ", pi->doc_id, pi->tf);

		{
			pos_arr = pi->pos_arr;
			printf("positions: [");
			for (i = 0; i < pi->tf; i++)
				printf("%u ", pos_arr[i]);
			printf("] ");
		}
		printf("\n");

		if (!jumped && pi->doc_id == fromID) {
			printf("trying to jump to %lu...\n", jumpID);
			jumped = postlist_jump(po, jumpID);
			printf("jump: %s.\n", jumped ? "successful" : "failed");
			goto print_current;
		}

	} while (!jumped && postlist_next(po));

	postlist_finish(po);
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

	postlist_print_info(po);

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

	printf("Theoretical compact list size: %.2f KB\n",
	       (N * sizeof(int) * (2 + 5)) / 1024.f);

	mhook_print_unfree();
	return 0;
}
