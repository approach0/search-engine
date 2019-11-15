#include <assert.h>

inline static void print_pos_arr(position_t *pos_arr, uint n)
{
	printf(", pos=");
	for (uint k = 0; k < n; k++)
		printf("%d%s", pos_arr[k], (k == n - 1) ? "." : ", ");
}

inline static void print_inmemo_term_items(invlist_iter_t iter)
{
	struct term_posting_item item;
	printf("in-memo: ");

	do {
		uint64_t key = invlist_iter_curkey(iter);
		size_t rd_sz = invlist_iter_read(iter, &item);
		(void)rd_sz;
		(void)key;
		assert(key == item.doc_id);

		printf("[docID=%u, tf=%u", item.doc_id, item.tf);
		/* read term positions in this document */
		print_pos_arr(item.pos_arr, item.n_occur);
		printf("]");
	} while (invlist_iter_next(iter));

	invlist_iter_free(iter);
	printf("\n");
}

inline static void print_ondisk_term_items(void *reader)
{
	printf("on-disk: ");
	term_posting_start(reader);

	do {
		/* test both "get_cur_item" functions */
		struct term_posting_item pi;
		uint64_t doc_id = term_posting_cur(reader);
		term_posting_read(reader, &pi);

		(void)doc_id;
		assert(doc_id == pi.doc_id);

		printf("[docID=%u, tf=%u", pi.doc_id, pi.tf);

		/* read term positions in this document */
		print_pos_arr(pi.pos_arr, pi.n_occur);
		printf("]");

	} while (term_posting_next(reader));

	term_posting_finish(reader);
	printf("\n");
}
