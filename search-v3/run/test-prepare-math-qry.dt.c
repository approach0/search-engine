#include "mhook/mhook.h"

#include "merger/mergers.h"
#include "prepare-math-qry.h"

/* use MaxScore merger */
typedef struct ms_merger *merger_set_iter_t;
#define merger_set_iterator  ms_merger_iterator
#define merger_set_iter_next ms_merger_iter_next
#define merger_set_iter_free ms_merger_iter_free

static void keyprint(uint64_t k)
{
	printf("#%u, #%u, r%u", key2doc(k), key2exp(k), key2rot(k));
}

static void print_item(struct math_invlist_item *item)
{
	printf("#%u, #%u (exp%u, rot%u), w%u, w%u, o%u\n", item->docID,
		item->secID, item->expID, item->sect_root,
		item->sect_width, item->orig_width, item->symbinfo_offset);
}

int main()
{
	math_index_t index = math_index_open("../math-index-v3/tmp", "r");

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	struct math_qry mq;
	if (0 != math_qry_prepare(index, "k(b+a)+ab+\\sqrt{b}+k+a", &mq)) {
		printf("error!\n");
		goto skip;
	}

	foreach (iter, merger_set, &mq.merge_set) {
		struct math_invlist_item item;
		for (int i = 0; i < iter->size; i++) {
			merger_map_call(iter, read, i, &item, sizeof(item));
			print_item(&item);
		}

		ms_merger_iter_print(iter, keyprint);
		printf("\n");
	}

skip:
	math_qry_release(&mq);
	math_index_close(index);
	mhook_print_unfree();
	return 0;
}
