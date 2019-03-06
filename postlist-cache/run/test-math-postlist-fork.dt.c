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
		printf("doc#%u, exp#%u, ", item->doc_id, item->exp_id);
		printf("lr#%u, %u paths:\n", item->n_lr_paths, item->n_paths);

		for (int i = 0; i < item->n_paths; i++) {
			if (gener) {
				printf("\t gener pathinfo %u, %u, 0x%x, 0x%x, 0x%lx \n",
					item->wild_id[i],
					item->subr_id[i],
					item->tr_hash[i],
					item->op_hash[i],
					item->wild_leaves[i]
				);
			} else {
				printf("\t normal pathinfo %u, %u, %u, 0x%x \n",
					item->wild_id[i],
					item->subr_id[i],
					item->tr_hash[i],
					item->op_hash[i]
				);
			}
		}
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

	if (math_posting_type(disk_po) == TYPE_PREFIX)
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
