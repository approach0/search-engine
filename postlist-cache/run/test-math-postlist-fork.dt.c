#include <stdio.h>
#include "mhook/mhook.h"
#include "postlist/postlist.h" /* for print_postlist() */
#include "math-index/math-index.h"
#include "math-postlist-cache.h"

void test_mem_post(struct postlist *po)
{
	foreach (iter, postlist, po) {
		struct math_posting_item *item;
		item = postlist_iter_cur_item(iter);
		printf("%u, %u\n", item->doc_id, item->exp_id);
	}
}

int main()
{
	struct postlist *mem_po;
	math_posting_t *disk_po;
	const char path[] = 
		"/home/tk/index-fix-decimal-and-use-latexml/prefix/VAR/TIMES/GTLS/";

	if (!math_posting_exits(path)) {
		return 1;
	}

	disk_po = math_posting_new_reader(path);
	if (!math_posting_start(disk_po)) {
		return 2;
	}

	mem_po = fork_math_postlist(disk_po);

	test_mem_post(mem_po);

	postlist_free(mem_po);

	math_posting_finish(disk_po);
	math_posting_free_reader(disk_po);

	mhook_print_unfree();
	return 0;
}
