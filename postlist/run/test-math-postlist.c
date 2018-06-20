#include <assert.h>
#include <stdio.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "postlist-codec/postlist-codec.h"
#include "postlist.h"
#include "math-postlist.h"

static void test_iterator(struct postlist *po, struct postlist_codec_fields fields)
{
	struct math_postlist_item *pi;
	postlist_start(po);

	do {
		pi = postlist_cur_item(po);
	//	postlist_print(pi, 1, fields);

	} while (postlist_next(po));

	postlist_finish(po);
	printf("\n");
}

static void test(struct postlist *po, int test_n)
{
	struct math_postlist_item *items;
	size_t fl_sz;

	/* generate random items */
	PTR_CAST(codec, struct postlist_codec, po->buf_arg);
	items = postlist_random(test_n, codec->fields);

	/* print fields and items */
	postlist_print_fields(codec->fields);
	//postlist_print(items, test_n, codec->fields);

	/* append items to posting list */
	for (int i = 0; i < test_n; i++) {
		struct math_postlist_item *item = items + i;
		fl_sz = postlist_write(po, item, sizeof(*item));
		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);
	}

	fl_sz = postlist_write_complete(po);
	printf("final flush %lu bytes.\n", fl_sz);

	/* print posting list information */
	postlist_print_info(po);

	/* test posting list iterator */
	test_iterator(po, codec->fields);

	/* free random items */
	free(items);
}


int main()
{
	struct postlist *po;
	const int test_n = 2000;

	printf("[[[ math_postlist_create_plain ]]]\n");
	po = math_postlist_create_plain();
	test(po, test_n);
	postlist_free(po);

	printf("[[[ math_postlist_create_compressed ]]]\n");
	po = math_postlist_create_compressed();
	test(po, test_n);
	postlist_free(po);

	printf("Theoretical compact list size: %.2f KB\n",
	       (test_n * sizeof(int) * (4 + 5*3)) / 1024.f);

	mhook_print_unfree();
	return 0;
}
