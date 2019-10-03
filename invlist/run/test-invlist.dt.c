#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "mhook/mhook.h"

#include "codec/codec.h"
#include "invlist.h"

#include <assert.h>

/* for memcmp to work as expected, we have to avoid optimal alignment */
#pragma pack(push, 1)
struct math_invlist_item {
	uint32_t docID;
	uint32_t secID;
	uint8_t  sect_width;
	uint8_t  orig_width;
	uint32_t symbinfo_offset; /* pointing to symbinfo file offset */
};
#pragma pack(pop)

/* field index for math_invlist_item */
enum {
	FI_DOCID,
	FI_SECID,
	FI_SECT_WIDTH,
	FI_ORIG_WIDTH,
	FI_OFFSET
};

static void print_item(struct math_invlist_item *item)
{
	printf("#%u, #%u, w%u, w%u, o%u\n", item->docID, item->secID,
		item->sect_width, item->orig_width, item->symbinfo_offset);
}

static struct invlist *
gen_random_items(const char *path, struct codec_buf_struct_info *info,
	struct math_invlist_item items[], int N)
{
	/* create invert list and iterator */
	struct invlist *invlist = invlist_open(path, MATH_INDEX_BLK_LEN, info);
	invlist_iter_t writer = invlist_writer(invlist);

	assert(invlist_empty(invlist));

	/* write a sequence of random items */
	size_t flush_sz;
	srand(time(0));

	uint last_docID = 1;
	uint last_offset = 0;
	for (int i = 0; i < N; i++) {
		items[i].docID = last_docID + rand() % 10;
		items[i].secID = rand() % 10;
		items[i].sect_width = rand() % 6;
		items[i].orig_width = rand() % 64;
		items[i].symbinfo_offset = last_offset + rand() % 128;

		flush_sz = invlist_writer_write(writer, items + i);
		printf("write items[%d] (flush %lu): ", i, flush_sz);
		print_item(items + i);

		last_docID = items[i].docID;
		last_offset = items[i].symbinfo_offset;
	}

	flush_sz = invlist_writer_flush(writer);
	printf("final flush ... (flush size = %lu) \n", flush_sz);

	invlist_writer_free(writer);
	return invlist;
}

static void
test_iterator(struct invlist *invlist, struct math_invlist_item items[])
{
	/* test iterator reader */
	int i = 0, err_cnt = 0;
	foreach (iter, invlist, invlist) {
		size_t rd_sz;
		struct math_invlist_item item;

		rd_sz = invlist_iter_read(iter, &item);
		(void)rd_sz;
		printf("read %lu bytes, ", rd_sz);
		print_item(&item);

		if (memcmp(items + i, &item, sizeof item) != 0) {
			err_cnt ++;
			prerr("failed.");
		} else {
			prinfo("pass.");
		}

		i++;
	}

	if (0 == err_cnt)
		prinfo("all pass.");
}

static void
test_skipping(struct invlist *invlist)
{
	/* test utility reader */
	invlist_print_as_decoded_ints(invlist);

	uint64_t from, to;
	printf("enter from, to ...\n");
	scanf("%lu, %lu", &from, &to);

	foreach (iter, invlist, invlist) {
		uint64_t key = iter->bufkey(iter, iter->buf_idx);
		if (key == from) {
			struct math_invlist_item item;

			invlist_iter_read(iter, &item);
			printf("\n FROM \n");
			printf("[%14lu]: ", key);
			print_item(&item);

			invlist_iter_jump(iter, to);

			invlist_iter_read(iter, &item);
			printf("\n JUMP TO \n");
			key = iter->bufkey(iter, iter->buf_idx);
			printf("[%14lu]: ", key);
			print_item(&item);
		}
	}
}

int main()
{
	/* fill in structure field information */
	struct codec_buf_struct_info *info;
	info = codec_buf_struct_info_alloc(5, sizeof(struct math_invlist_item));

#define SET_FIELD_INFO(_idx, _name, _codec) \
	info->field_info[_idx] = FIELD_INFO(struct math_invlist_item, _name, _codec)

	SET_FIELD_INFO(FI_DOCID, docID, CODEC_FOR_DELTA);
	SET_FIELD_INFO(FI_SECID, secID, CODEC_FOR);
	SET_FIELD_INFO(FI_SECT_WIDTH, sect_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_ORIG_WIDTH, orig_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_OFFSET, symbinfo_offset, CODEC_FOR_DELTA);

#define N (30)
	struct invlist *invlist;
	struct math_invlist_item items[N] = {0};

//	/* test for in-memory inverted list */
//	invlist = gen_random_items(NULL, info, items, N);
//	// test_iterator(invlist, items);
//	test_skipping(invlist);
//	invlist_free(invlist);

	/* test for on-disk inverted list */
	invlist = gen_random_items("run/invlist", info, items, N);
//	test_iterator(invlist, items);
	test_skipping(invlist);
	invlist_free(invlist);

	/* free structure field information */
	codec_buf_struct_info_free(info);
	mhook_print_unfree();
	return 0;
}
