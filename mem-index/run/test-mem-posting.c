#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "mem-posting.h"
#include "config.h"

#undef N_DEBUG
#include <assert.h>

enum test_option {
	TEST_PLAIN_POSTING,
	TEST_CODEC_POSTING,
	TEST_CODEC_POSTING_WITH_POS
};

static struct term_posting_item
*test_gen_item(const doc_id_t docID, size_t *alloc_sz, enum test_option opt)
{
	struct term_posting_item *pi;
	position_t *pos;
	uint32_t i, n;

	if (opt == TEST_CODEC_POSTING_WITH_POS)
		n = 1 + rand() % 4;
	else
		n = 0;

	*alloc_sz = sizeof(struct term_posting_item) + n * sizeof(position_t);
	pi = malloc(*alloc_sz);

	pi->doc_id = docID;
	pi->tf = n;

	printf("(docID=%u, tf=%u) ", docID, n);

	if (opt == TEST_CODEC_POSTING_WITH_POS) {
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


static void test_gen_data(struct mem_posting *po, enum test_option opt)
{
	struct   term_posting_item *pi;
	size_t   alloc_sz;
	size_t   fl_sz;
	doc_id_t i;

	for (i = 1; i < 20; i++) {
		pi = test_gen_item(i, &alloc_sz, opt);

		printf("writing %lu bytes...\n", alloc_sz);
		fl_sz = mem_posting_write(po, pi, alloc_sz);

		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);

		free(pi);
	}

	fl_sz = mem_posting_write_complete(po);
	printf("flush %lu bytes.\n", fl_sz);
}

static void test_iterator(struct mem_posting *po, enum test_option opt)
{
	struct term_posting_item *pi;
	position_t *pos_arr;
	uint32_t i;
	uint64_t id;
	uint32_t fromID;
	uint64_t jumpID;
	int      jumped = 0;

	printf("Please input (from, jump_to) IDs:\n");
	scanf("%u, %lu", &fromID, &jumpID);

	mem_posting_start(po);

	do {
print_current:
		pi = mem_posting_cur_item(po);
		id = mem_posting_cur_item_id(pi);
		assert(id == pi->doc_id);

		printf("[docID=%u, tf=%u] ", pi->doc_id, pi->tf);

		if (opt == TEST_CODEC_POSTING_WITH_POS) {
			pos_arr = mem_posting_cur_pos_arr(po);
			printf("positions: [");
			for (i = 0; i < pi->tf; i++)
				printf("%u ", pos_arr[i]);
			printf("] ");
			free(pos_arr);
		}

		if (!jumped && pi->doc_id == fromID) {
			printf("trying to jump to %lu...\n", jumpID);
			jumped = mem_posting_jump(po, jumpID);
			printf("jump: %s.\n", jumped ? "successful" : "failed");
			goto print_current;
		}

	} while (!jumped && mem_posting_next(po));

	mem_posting_finish(po);
	printf("\n");

	mem_posting_free(po);
}

static void run_testcase(enum test_option opt)
{
	struct mem_posting *po;
	srand(time(NULL));

	switch (opt) {
	case TEST_PLAIN_POSTING:
		po = mem_posting_create(2, mem_term_posting_plain_calls());
		break;
	case TEST_CODEC_POSTING:
		po = mem_posting_create(2, mem_term_posting_codec_calls());
		break;
	case TEST_CODEC_POSTING_WITH_POS:
		po = mem_posting_create(2, mem_term_posting_with_pos_codec_calls());
		break;
	default:
		assert(0);
	};

	test_gen_data(po, opt);

	mem_posting_print_info(po);

	test_iterator(po, opt);

	printf("\n");
}

int main()
{
	printf("{{{ TEST_PLAIN_POSTING }}}\n");
	run_testcase(TEST_PLAIN_POSTING);

	printf("{{{ TEST_CODEC_POSTING }}}\n");
	run_testcase(TEST_CODEC_POSTING);

	printf("{{{ TEST_CODEC_POSTING_WITH_POS }}}\n");
	run_testcase(TEST_CODEC_POSTING_WITH_POS);

	return 0;
}
