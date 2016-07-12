#include <string.h>
#include "mem-posting.h"

#undef N_DEBUG
#include <assert.h>

int main()
{
	uint32_t i;
	size_t fl_sz;
	struct mem_posting *po;
	uint64_t id;
	struct term_posting_item *pi, item = {0, 0};

	uint32_t fromID;
	uint64_t jumpID;
	int      jumped = 0;

	struct mem_posting_callbks calls = {
		mem_posting_default_on_flush,
		mem_posting_default_on_rebuf,
		mem_posting_default_get_pos_arr
	};

	po = mem_posting_create(2, calls);

	for (i = 1; i < 20; i++) {
		item.doc_id += i;
		item.tf = i % 3;
		printf("writting item (%u, %u)...\n", item.doc_id, item.tf);

		fl_sz = mem_posting_write(po, &item, sizeof(struct term_posting_item));
		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);
	}

	mem_posting_write_complete(po);

	mem_posting_print_info(po);

	/* test iterator */
	printf("Please input (from, jump_to) IDs:\n");
	scanf("%u, %lu", &fromID, &jumpID);

	mem_posting_start(po);
	printf("inital cur=%p\n", po->cur);

	do {
print_current:
		pi = mem_posting_current(po);
		id = mem_posting_current_id(pi);
		assert(id == pi->doc_id);

		printf("[docID=%u, tf=%u] ", pi->doc_id, pi->tf);

		if (!jumped && pi->doc_id == fromID) {
			printf("trying to jump to %lu...\n", jumpID);
			jumped = mem_posting_jump(po, jumpID);
			printf("jump: %s.\n", jumped ? "successful" : "failed");
			goto print_current;
		}

	} while (!jumped && mem_posting_next(po));

	mem_posting_finish(po);
	printf("\n");

	/* free posting list */
	mem_posting_free(po);
	return 0;
}
