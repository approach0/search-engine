#include <string.h>
#include "codec/codec.h"
#include "mem-posting.h"
#include "config.h"

#undef N_DEBUG
#include <assert.h>

static char *mem_posting_get_pos_arr(char *buf, size_t *size)
{
	uint32_t *tf = (uint32_t *)buf + 1;
	*size = 0;
	return (char *)(tf + 1);
}

//static void print(char *p, size_t sz)
//{
//	int i;
//	for (i = 0; i < sz; i++) {
//		printf("%x ", p[i]);
//	}
//	printf("]]]\n");
//}

static uint32_t mem_posting_on_flush(char *buf, uint32_t *buf_sz)
{
	uint32_t tmp_docID[MEM_POSTING_BUF_SZ];
	uint32_t tmp_tf[MEM_POSTING_BUF_SZ];
	uint32_t *tf, n, offset = 0;
	doc_id_t *docID, key;
	uint32_t *head = (uint32_t *)buf;
	char *cur = buf;
	char *in;

	key = *(doc_id_t *)buf;

	size_t res;
	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	for (n = 0; offset != *buf_sz; n++) {
		docID = (doc_id_t *)cur;
		tf = (uint32_t *)(docID + 1);
		cur = (char *)(tf + 1);

		tmp_docID[n] = *docID;
		tmp_tf[n] = *tf;

		offset = (uint32_t)(cur - buf);
		assert(offset <= *buf_sz);
	}

	*head = n;
	in = (char *)(head + 1);

	res = codec_compress_ints(codec[0], tmp_docID, n, in);
	res += codec_compress_ints(codec[1], tmp_tf, n, in + res);

	assert(res <= MEM_POSTING_BUF_SZ);
	*buf_sz = res + 1;

	free(codec[0]);
	free(codec[1]);

	return key;
}

static void mem_posting_on_rebuf(char *buf, uint32_t *buf_sz)
{
	uint32_t *head = (uint32_t *)buf;
	uint32_t tmp_docID[MEM_POSTING_BUF_SZ];
	uint32_t tmp_tf[MEM_POSTING_BUF_SZ];
	uint32_t i, n;
	size_t res, ori_sz;
	char *in;

	doc_id_t *docID;
	uint32_t *tf;

	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	n = *head;
	in = (char *)(head + 1);

	res = codec_decompress_ints(codec[0], in, tmp_docID, n);
	assert(res <= MEM_POSTING_BUF_SZ);
	res = codec_decompress_ints(codec[1], in + res, tmp_tf, n);
	assert(res <= MEM_POSTING_BUF_SZ);

	ori_sz = 0;
	docID = (doc_id_t *)buf;
	for (i = 0; i < n; i++) {
		*docID = tmp_docID[i];
		tf = (uint32_t *)(docID + 1);
		*tf = tmp_tf[i];
		docID = (doc_id_t *)(tf + 1);

		ori_sz += sizeof(uint32_t) + sizeof(doc_id_t);
	}

	*buf_sz = ori_sz;

	free(codec[0]);
	free(codec[1]);
	return;
}

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
		mem_posting_on_flush,
		mem_posting_on_rebuf,
		mem_posting_get_pos_arr,
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

	fl_sz = mem_posting_write_complete(po);
	printf("flush %lu bytes.\n", fl_sz);

	mem_posting_print_info(po);

	/* test iterator */
	printf("Please input (from, jump_to) IDs:\n");
	scanf("%u, %lu", &fromID, &jumpID);

	mem_posting_start(po);

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
