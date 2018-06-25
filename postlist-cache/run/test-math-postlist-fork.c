#include <stdio.h>
#include "mhook/mhook.h"
#include "postlist/postlist.h" /* for print_postlist() */
#include "math-index/math-index.h"
#include "math-postlist-cache.h"

int main()
{
	math_posting_t *disk_po;
	struct postlist *mem_po;
	disk_po = math_posting_new_reader(
		"/home/tk/index-fix-decimal-and-use-latexml/prefix/ONE/ADD"
	);

	if (!math_posting_start(disk_po)) {
		/* this directory does not have index file */
		return 1;
	}

	mem_po = fork_math_postlist(disk_po);
	// print_postlist(mem_po);
	postlist_free(mem_po);

	math_posting_finish(disk_po);
	math_posting_free_reader(disk_po);

	mhook_print_unfree();
	return 0;
}
