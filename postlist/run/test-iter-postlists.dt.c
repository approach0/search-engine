#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "postlist-codec/postlist-codec.h"
#include "postlist.h"
#include "math-postlist.h"

static void gen_random(struct postlist *po, int test_n)
{
	struct math_postlist_item *items;

	/* generate random array of items */
	PTR_CAST(codec, struct postlist_codec, po->buf_arg);
	items = postlist_random(test_n, codec->fields);

	{ /* print truth */
		PTR_CAST(codec, struct postlist_codec, po->buf_arg);
		postlist_print(items, test_n, codec->fields);
	}

	/* append items to posting list */
	for (int i = 0; i < test_n; i++) {
		struct math_postlist_item *item = items + i;
		postlist_write(po, item, sizeof(*item));
	}

	postlist_write_complete(po);

	/* free random items */
	free(items);
}

static void test_iters(struct postlist *po)
{
	if (!postlist_empty(po)) {
		struct postlist_iterator *iter1 = postlist_iterator(po);
		struct postlist_iterator *iter2 = postlist_iterator(po);
		int cnt = 0;
		do {
			struct math_postlist_item *item;

			item = postlist_iter_cur_item(iter1);
			printf("iter1: %u, %u\n", item->doc_id, item->exp_id);

			item = postlist_iter_cur_item(iter2);
			printf("iter2: %u, %u\n", item->doc_id, item->exp_id);
			printf("\n");

			if (random() % 2) {
				printf("iter1 advances\n");
				postlist_iter_next(iter1);
			} else {
				printf("iter2 advances\n");
				postlist_iter_next(iter2);
			}
			cnt ++;
		} while (cnt < 20);

		postlist_iter_free(iter1);
		postlist_iter_free(iter2);
	}
}

int main()
{
	struct postlist *po;
	const int test_n = 20;

	printf("=== math_postlist_create_plain ===\n");
	po = math_postlist_create_plain();
	gen_random(po, test_n);
	test_iters(po);
	postlist_free(po);

	printf("=== math_postlist_create_compressed ===\n");
	po = math_postlist_create_compressed();
	gen_random(po, test_n);
	test_iters(po);
	postlist_free(po);

	mhook_print_unfree();
	return 0;
}
