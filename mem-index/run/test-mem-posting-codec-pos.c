#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "codec/codec.h"
#include "mem-posting.h"
#include "config.h"

#undef N_DEBUG
#include <assert.h>

static char *mem_posting_get_pos_arr(char *buf, size_t *size)
{
	uint32_t *tf = (uint32_t *)buf + 1;
	*size = sizeof(position_t) * (*tf);
	return (char *)(tf + 1);
}

static uint32_t mem_posting_on_flush(char *buf, uint32_t *buf_sz)
{
	uint32_t tmp_docID[MEM_POSTING_BUF_SZ];
	uint32_t tmp_tf[MEM_POSTING_BUF_SZ];
	uint32_t tmp_pos[MEM_POSTING_BUF_SZ];

	uint32_t *tf, n, tot_tf = 0, pos_idx = 0, offset = 0;
	doc_id_t *docID, key;
	uint32_t *head = (uint32_t *)buf;
	char *cur = buf;
	char *in;
	size_t pos_sz;

	key = *(doc_id_t *)buf;

	size_t res;
	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	for (n = 0; offset != *buf_sz; n++) {
		docID = (doc_id_t *)cur;
		tf = (uint32_t *)(docID + 1);
		cur = (char *)(tf + 1);

		tmp_docID[n] = *docID;
		tmp_tf[n] = *tf;

		pos_sz = (*tf) + sizeof(position_t);
		memcpy(tmp_pos + pos_idx, cur, pos_sz);
		tot_tf += tmp_tf[n];
		pos_idx += pos_sz;
		cur += pos_sz;

		offset = (uint32_t)(cur - buf);
		assert(offset <= *buf_sz);
	}

	*head = n;
	in = (char *)(head + 1);

	res = codec_compress_ints(codec[0], tmp_docID, n, in);
	res += codec_compress_ints(codec[1], tmp_tf, n, in + res);
	res += codec_compress_ints(codec[2], tmp_pos, tot_tf, in + res);

	assert(res <= MEM_POSTING_BUF_SZ);
	*buf_sz = res + 1;

	free(codec[0]);
	free(codec[1]);
	free(codec[2]);

	return key;
}

static void mem_posting_on_rebuf(char *buf, uint32_t *buf_sz)
{
	uint32_t *head = (uint32_t *)buf;
	uint32_t tmp_docID[MEM_POSTING_BUF_SZ];
	uint32_t tmp_tf[MEM_POSTING_BUF_SZ];
	uint32_t tmp_pos[MEM_POSTING_BUF_SZ];
	uint32_t i, j, n, pos_idx = 0, tot_tf = 0;
	size_t ori_sz;
	char *in;

	doc_id_t *docID;
	uint32_t *tf;
	position_t *pos;

	struct codec *codec[] = {
		codec_new(CODEC_FOR_DELTA, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS),
		codec_new(CODEC_FOR, CODEC_DEFAULT_ARGS)
	};

	n = *head;
	in = (char *)(head + 1);

	in += codec_decompress_ints(codec[0], in, tmp_docID, n);
	in += codec_decompress_ints(codec[1], in, tmp_tf, n);

	for (i = 0; i < n; i++)
		tot_tf += tmp_tf[i];
	in += codec_decompress_ints(codec[2], in, tmp_pos, tot_tf);

	ori_sz = 0;
	docID = (doc_id_t *)buf;
	for (i = 0; i < n; i++) {
		*docID = tmp_docID[i];
		tf = (uint32_t *)(docID + 1);
		*tf = tmp_tf[i];

		pos = (position_t *)(tf + 1);
		for (j = 0; j < *tf; j++)
			pos[j] = tmp_pos[pos_idx + j];
		pos_idx += *tf;

		docID = (doc_id_t *)(pos + pos_idx);

		ori_sz += sizeof(uint32_t) + sizeof(doc_id_t) + (*tf) * sizeof(position_t);
	}

	assert(ori_sz <= MEM_POSTING_BUF_SZ);
	*buf_sz = ori_sz;

	free(codec[0]);
	free(codec[1]);
	free(codec[2]);
	return;
}

static struct term_posting_item
*gen_rand_item(const doc_id_t docID, size_t *alloc_sz)
{
	struct term_posting_item *pi;
	position_t *pos;
	uint32_t i, n = 1 + rand() % 4;

	*alloc_sz = sizeof(struct term_posting_item) + n * sizeof(position_t);
	pi = malloc(*alloc_sz);
	pos = (position_t *)(pi + 1);

	pi->doc_id = docID;
	pi->tf = n;

	printf("(docID=%u, tf=%u) ", docID, n);

	pos[0] = rand() % 10;
	for (i = 1; i < n; i++)
		pos[i] = pos[i - 1] + rand() % 100;

	printf("positions: [");
	for (i = 0; i < n; i++)
		printf("%u ", pos[i]);
	printf("]\n");

	return pi;
}

int main()
{
	uint32_t i;
	size_t fl_sz;
	struct mem_posting *po;
	position_t *pos_arr;
	uint64_t id;
	struct term_posting_item *pi;
	size_t alloc_sz;

	uint32_t fromID;
	uint64_t jumpID;
	int      jumped = 0;

	struct mem_posting_callbks calls = {
		mem_posting_default_on_flush,
		mem_posting_default_on_rebuf,
		mem_posting_get_pos_arr
	};

	po = mem_posting_create(2, calls);

	srand(time(NULL));

	for (i = 1; i < 20; i++) {
		pi = gen_rand_item(i, &alloc_sz);
		fl_sz = mem_posting_write(po, pi, alloc_sz);
		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);
		free(pi);
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
		pos_arr = mem_posting_cur_pos_arr(po);
		id = mem_posting_current_id(pi);
		assert(id == pi->doc_id);

		printf("[docID=%u, tf=%u] ", pi->doc_id, pi->tf);
		printf("positions: [");
		for (i = 0; i < pi->tf; i++)
			printf("%u ", pos_arr[i]);
		printf("] ");

		free(pos_arr);

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
