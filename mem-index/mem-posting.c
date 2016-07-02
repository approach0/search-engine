#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "mem-posting.h"
#include "config.h"

#undef NDEBUG
#include <assert.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))

typedef void (*blk_print_callbk)(struct mem_posting*, struct mem_posting_blk*);

static void print_int32_buff(uint32_t *integer, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
		printf("%u,", integer[i]);
	printf("\n");
}

static void print_int8_buff(uint8_t *str, size_t n)
{
	size_t i;
	for (i = 0; i < n; i++)
		printf("%x,", str[i]);
	printf("\n");
}

static void po_init(struct mem_posting *po, uint32_t skippy_spans)
{
	po->n_used_bytes = 0;
	po->n_tot_blocks = 0;
	po->head = NULL; /* lazy, create on write */
	po->tail = NULL;
	skippy_init(&po->skippy, skippy_spans);

	/* encoder */
	po->struct_sz = 0;
	po->codecs = NULL;

	/* leave merge-related initializations to mem_posting_start() */
}

struct mem_posting *mem_posting_create(uint32_t skippy_spans)
{
	struct mem_posting *ret;
	ret = malloc(sizeof(struct mem_posting));
	po_init(ret, skippy_spans);

#ifdef DEBUG_MEM_POSTING
	printf("new memory posting list created.\n");
#endif
	return ret;
}

void mem_posting_clear(struct mem_posting *po)
{
	uint32_t skippy_spans = po->skippy.n_spans;
	struct mem_posting_blk *save, *p = po->head;

	while (p) {
		save = p->next;
#ifdef DEBUG_MEM_POSTING
	printf("free memory posting list block.\n");
#endif
		free(p);
		p = save;
	}
	//Or use skippy_free(&po->skippy, struct mem_posting_blk, sn);

	po_init(po, skippy_spans);
}

void mem_posting_release(struct mem_posting *po)
{
	uint32_t i, n_members = po->struct_sz / sizeof(uint32_t);

	/* free codecs */
	for (i = 0; i < n_members; i++)
		codec_free(po->codecs[i]);
	free(po->codecs);

	/* free posting list blocks */
	mem_posting_clear(po);

	/* free posting list header */
#ifdef DEBUG_MEM_POSTING
	printf("free memory posting list.\n");
#endif
	free(po);
}

static struct mem_posting_blk *create_blk(uint32_t key)
{
	struct mem_posting_blk *ret;
	ret = malloc(sizeof(struct mem_posting_blk));

	skippy_node_init(&ret->sn, key);

	ret->end = 0;
	ret->n_writes = 0;
	ret->next = NULL;

	return ret;
}

static int
append_blk(struct mem_posting *po, struct mem_posting_blk *blk)
{
	if (po->head == NULL)
		po->head = blk;
	else
		po->tail->next = blk;

	po->tail = blk;

	po->n_tot_blocks ++;

#ifdef DEBUG_MEM_POSTING
	printf("new block appended to memory posting list.\n");
#endif
	return 0;
}

static __inline bool is_paging(struct mem_posting *po, size_t next_bytes)
{
	return (po->tail == NULL /* no block created yet */ ||
	        po->tail->end + next_bytes > MEM_POSTING_BLOCK_SZ
	        /* not enough space */);
}

void mem_posting_set_codecs(struct mem_posting *po, uint32_t struct_sz,
                            struct codec **codecs)
{
	uint32_t i, n_members = struct_sz / sizeof(uint32_t);
	po->struct_sz = struct_sz;

	/* copy codecs */
	po->codecs = calloc(n_members, sizeof(struct codec*));
	for (i = 0; i < n_members; i++)
		po->codecs[i] = codec_new(codecs[i]->method, codecs[i]->args);
}

uint32_t
mem_posting_encode(struct mem_posting *dest, struct mem_posting *src)
{
	struct mem_posting_blk *srcblk = src->head;
	uint32_t struct_sz = dest->struct_sz;
	/* n_encode is the number of structure to be encoded at a time */
	const uint32_t n_encode = MAX(1, MEM_POSTING_N_ENCODE);
	const uint32_t n_encode_bytes = n_encode * struct_sz;

	uint32_t  byte_now, first_key, res_bytes, remain_bytes, n_enc_remain;
	char      buf[MEM_POSTING_BLOCK_SZ];
	enc_hd_t *buf_head = (enc_hd_t*)buf;
	enc_hd_t *buf_load = buf_head + 1;

	while (srcblk) {
		/* for each of source block in order */
		byte_now = 0;
		n_enc_remain = srcblk->n_writes;

		while (byte_now < srcblk->end) {
			/* for each N structures in this source block */
			first_key = *(uint32_t*)(srcblk->buff + byte_now);
			remain_bytes = srcblk->end - byte_now;

#ifdef DEBUG_MEM_POSTING_ENC
			printf("DEBUG\n");
			printf("n_encode = %u\n", n_encode);
			printf("remain_bytes = %u\n", remain_bytes);
#endif

			if (remain_bytes >= n_encode_bytes) {
#ifdef DEBUG_MEM_POSTING_ENC
				printf("case A\n");
				printf("[before encode %u * %u]\n", struct_sz, n_encode);
				print_int8_buff((uint8_t*)srcblk->buff + byte_now,
				                struct_sz * n_encode);
#endif

				/* encode n_encode structures */
				res_bytes = encode_struct_arr(buf_load,
				                              srcblk->buff + byte_now,
				                              dest->codecs, n_encode,
				                              struct_sz);
				byte_now += n_encode_bytes;
				n_enc_remain -= n_encode;

				/* write encode header */
				*buf_head = n_encode;
			} else {
#ifdef DEBUG_MEM_POSTING_ENC
				printf("case B\n");
				printf("[before encode %u * %u]\n", struct_sz, n_enc_remain);
				print_int8_buff((uint8_t*)srcblk->buff + byte_now,
				                struct_sz * n_enc_remain);
#endif

				/* encode n_enc_remain structures */
				res_bytes = encode_struct_arr(buf_load,
				                              srcblk->buff + byte_now,
				                              dest->codecs, n_enc_remain,
				                              struct_sz);
				byte_now += remain_bytes;

				/* write encode header */
				*buf_head = n_enc_remain;
			}

#ifdef DEBUG_MEM_POSTING_ENC
			printf("[after encode]\n");
			print_int8_buff((uint8_t*)buf_load, res_bytes);
#endif

			/* write encoded bytes to destination memory posting */
			mem_posting_write(dest, first_key, buf,
			                  sizeof(enc_hd_t) + res_bytes);
		}

		srcblk = srcblk->next;
	}

	return n_encode;
}

size_t
mem_posting_write(struct mem_posting *po, uint32_t first_key,
                  const void *in, size_t bytes)
{
	struct mem_posting_blk *blk;
	assert(bytes <= MEM_POSTING_BLOCK_SZ);

	if (is_paging(po, bytes)) {

		blk = create_blk(first_key);
		if (blk == NULL)
			return 0;

		skippy_append(&po->skippy, &blk->sn);
		append_blk(po, blk);
		return mem_posting_write(po, first_key, in, bytes);
	}

	memcpy(po->tail->buff + po->tail->end, in, bytes);
	po->tail->end += bytes;
	po->tail->n_writes ++;
	po->n_used_bytes += bytes;

	return 0;
}

void mem_posting_print_meminfo(struct mem_posting *po)
{
	uint32_t i, n_encode, n_members, struct_sz = po->struct_sz;
	uint32_t mem_usage = po->n_tot_blocks * MEM_POSTING_BLOCK_SZ;

	printf("%u blocks (%.2f KB), %.2f%% used.", po->n_tot_blocks,
	       (float)mem_usage / 1024.f, (mem_usage == 0) ? 0.f :
	       ((float)po->n_used_bytes / (float)mem_usage) * 100.f);

	if (struct_sz != 0) {
		n_encode = MAX(1, MEM_POSTING_N_ENCODE);
		printf(" (encoded, n_encode=%u)", n_encode);
	}
	printf("\n");

	if (po->codecs == NULL) {
		printf("no codec(s) specified.\n");
		return;
	}

	n_members = struct_sz / sizeof(uint32_t);
	for (i = 0; i < n_members; i++)
		printf("member %u: %s\n", i, codec_method_str(po->codecs[i]->method));
}

static void po_print(struct mem_posting *po, blk_print_callbk blk_print_fun)
{
	struct mem_posting_blk *blk = po->head;
	int i = 0;
	printf("==== memory posting list ====\n");
	mem_posting_print_meminfo(po);

	skippy_print(&po->skippy);

	while (blk) {
		printf("block #%d ", i);
		if (blk == po->head)
			printf("(head) ");
		else if (blk == po->tail)
			printf("(tail) ");

		printf("[%u/%d used]:\n", blk->end, MEM_POSTING_BLOCK_SZ);

		skippy_node_print(&blk->sn);
		blk_print_fun(po, blk);

		blk = blk->next;
		i ++;
	}
}

static void
po_print_raw_callbk(struct mem_posting *po, struct mem_posting_blk* blk)
{
#ifdef DEBUG_MEM_POSTING_ENC
	print_int8_buff((uint8_t*)blk->buff, blk->end);
#else
	print_int32_buff((uint32_t*)blk->buff,
	                 (blk->end + sizeof(uint32_t) - 1) / sizeof(uint32_t));
#endif
}

void mem_posting_print_raw(struct mem_posting *po)
{
	po_print(po, &po_print_raw_callbk);
}

static void
po_print_dec_callbk(struct mem_posting *po, struct mem_posting_blk* blk)
{
	uint32_t j = 0;
	enc_hd_t *enc_head;
	uint32_t n_members = po->struct_sz / sizeof(uint32_t);

	size_t   res_bytes;
	char     tmpbuf[MEM_POSTING_BLOCK_SZ];

	while (j < blk->end) {
		enc_head = (enc_hd_t*)(blk->buff + j);
		j += sizeof(enc_hd_t);

#ifdef DEBUG_MEM_POSTING_ENC
		printf("[before decode %u * %u]\n", po->struct_sz, (*enc_head));
		print_int8_buff((uint8_t*)blk->buff + j, po->struct_sz * (*enc_head));
#endif
		res_bytes = decode_struct_arr(tmpbuf, blk->buff + j, po->codecs,
		                              *enc_head, po->struct_sz);

		printf("[dec %lu bytes]: ", res_bytes);
		print_int32_buff((uint32_t*)(tmpbuf), (*enc_head) * n_members);

		j += res_bytes;
	}
}

void mem_posting_print_dec(struct mem_posting *po)
{
	po_print(po, &po_print_dec_callbk);
}

/*
 * posting merge related functions.
 */
static bool merge_next_blk(struct mem_posting *po)
{
	po->blk_now = po->blk_now->next;
	po->blk_idx = 0;
#ifdef DEBUG_MEM_POSTING
	printf("merge next block.\n");
#endif

	return (po->blk_now != NULL);
}

static void merge_rebuf(struct mem_posting *po)
{
	size_t enc_bytes /* number of encoded bytes (smaller number) */;
	size_t dec_bytes /* number of decoded bytes (larger number) */;

#ifdef DEBUG_MEM_POSTING
	printf("merge rebuf.\n");
#endif

	assert(po->blk_idx <= po->blk_now->end);

	if (po->blk_idx == po->blk_now->end) {
		/* go to a new block */
		if (!merge_next_blk(po)) {
			dec_bytes = 0;
			goto reset_buf_ptr;
		}
	}

	/* read encode header */
	enc_hd_t *enc_head = (enc_hd_t*)(po->blk_now->buff + po->blk_idx);
	po->blk_idx += sizeof(enc_hd_t);

#ifdef DEBUG_MEM_POSTING
	//printf("enc_head = %u, blk_idx = %u\n", *enc_head, po->blk_idx);
#endif

	/* decode into merge buffer */
	enc_bytes = decode_struct_arr(po->buff, po->blk_now->buff + po->blk_idx,
	                              po->codecs, *enc_head, po->struct_sz);
	dec_bytes = (*enc_head) * po->struct_sz;
	po->blk_idx += enc_bytes;

reset_buf_ptr:
	po->buf_idx = 0;
	po->buf_end = dec_bytes;
}

bool mem_posting_start(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	po->buff = NULL;

	if (po->head == NULL)
		return 0;

#ifdef DEBUG_MEM_POSTING
	printf("allocating posting merge buffer.\n");
#endif
	po->buff = malloc(MEM_POSTING_BLOCK_SZ);

	if (po->buff == NULL)
		return 0;

	po->blk_now = po->head;
	po->blk_idx = 0;

	/* read the very first decoded bytes into merge buffer */
	merge_rebuf(po);
	return (po->buf_end != 0);
}

bool mem_posting_next(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	po->buf_idx += po->struct_sz;

	do {
		if (po->buf_idx < po->buf_end)
			return 1;
		else
			merge_rebuf(po);

	} while (po->buf_end != 0);

	return 0;
}

bool mem_posting_jump(void *po_, uint64_t target_)
{
	uint32_t  target = (uint32_t)target_;
	uint32_t *curID;
	struct mem_posting *po = (struct mem_posting*)po_;
	struct skippy_node *jump_to;
	bool ret = 0;

	/* try to use skip-list first */
	jump_to = skippy_node_jump(&po->blk_now->sn, target);
#ifdef DEBUG_MEM_POSTING
	printf("skippy jumps to node %u.\n", jump_to->key);
#endif

	if (jump_to != &po->blk_now->sn) {
		/* if we can jump some blocks */
#ifdef DEBUG_MEM_POSTING
		printf("jump to a different node.\n");
#endif
		po->blk_now = MEMBER_2_STRUCT(jump_to, struct mem_posting_blk, sn);
		po->blk_idx = 0;
		merge_rebuf(po);
	}
#ifdef DEBUG_MEM_POSTING
	else
		printf("stay in the same node.\n");
#endif

	/* at this point, there shouldn't be any problem to access
	 * current posting item:
	 * case 1: if we check out to a new block, the merge buff
	 * must be non-empty.
	 * case 2: if we stay in the old block, it is guaranteed
	 * that current posting item is within legal scope. */

	/* seek in the current block until we get to an ID greater or
	 * equal to target ID. */
	do {
		/* docID must be the first member of structure */
		curID = (uint32_t*)mem_posting_current(po);

		if (*curID >= target) {
			ret = 1;
			break;
		}

	} while (mem_posting_next(po));

	return ret;
}

void mem_posting_finish(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;

	if (po->buff) {
#ifdef DEBUG_MEM_POSTING
		printf("free posting merge buffer.\n");
#endif
		free(po->buff);
		po->buff = NULL;
	}
}

void* mem_posting_current(void *po_)
{
	struct mem_posting *po = (struct mem_posting*)po_;
	return po->buff + po->buf_idx;
}

uint64_t mem_posting_current_id(void *item)
{
	uint32_t *curID = (uint32_t*)item;
	return (uint64_t)(*curID);
}
