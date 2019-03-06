#include <stdio.h>
#include "common/common.h"
#include "mhook/mhook.h"
#include "postlist/postlist.h" /* for print_postlist() */
#include "postlist/math-postlist.h" /* for math item print */
#include "math-index/math-index.h"
#include "math-postlist-cache.h"

void test_mem_post(struct postlist *po, int gener)
{
	foreach (iter, postlist, po) {
		struct math_postlist_gener_item *item;
		item = postlist_iter_cur_item(iter);

		math_postlist_print_item(item, gener);
	}
}

static int fork_from(char *path)
{
	struct postlist *mem_po;
	math_posting_t *disk_po;

	if (!math_posting_exits(path)) {
		return 1;
	}

	disk_po = math_posting_new_reader(path);
	if (!math_posting_start(disk_po)) {
		return 2;
	}

	mem_po = fork_math_postlist(disk_po);

	if (math_posting_type(disk_po) == MATH_PATH_TYPE_PREFIX)
		printf("prefix path uncompressed: %lu bytes / block (%lu items)\n",
			ROUND_UP(65536, sizeof(struct math_postlist_item)),
			ROUND_UP(65536, sizeof(struct math_postlist_item)) / sizeof(struct math_postlist_item)
		);
	else
		printf("gener path uncompressed: %lu bytes / block (%lu items)\n",
			ROUND_UP(65536, sizeof(struct math_postlist_gener_item)),
			ROUND_UP(65536, sizeof(struct math_postlist_gener_item)) / sizeof(struct math_postlist_gener_item)
		);
	printf("compressed: %lu bytes / block\n", (mem_po->head) ? mem_po->head->blk_sz : 0);
	printf("\n");

	if (math_posting_type(disk_po) == MATH_PATH_TYPE_PREFIX)
		test_mem_post(mem_po, 0);
	else
		test_mem_post(mem_po, 1);

	postlist_free(mem_po);

	math_posting_finish(disk_po);
	math_posting_free_reader(disk_po);
	return 0;
}

int main()
{
	fork_from("../math-index/tmp/gener/ADD/");
	printf("\n");
	fork_from("../math-index/tmp/prefix/VAR/");

	mhook_print_unfree();
}
