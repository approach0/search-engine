#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "common/common.h"
#include "invlist.h"
#include "config.h"
#include <assert.h>

/*
 * inverted list functions
 */

struct invlist
*invlist_open(const char *path, uint32_t n, codec_buf_struct_info_t* c_info)
{
	struct invlist *ret = malloc(sizeof *ret);

	if (path == NULL) {
		ret->head = NULL;
		ret->type = INVLIST_TYPE_INMEMO;
	} else {
		strcpy(ret->path, path);
		ret->type = INVLIST_TYPE_ONDISK;
	}

	ret->tot_payload_sz = 0;
	ret->n_blk = 0;
	skippy_init(&ret->skippy, n);
	ret->buf_max_len = n;
	ret->buf_max_sz = n * c_info->struct_sz;
	ret->c_info = c_info;

	/* use default bufkey callback */
	ret->bufkey = &invlist_iter_default_bufkey;

	return ret;
}

#include <unistd.h> /* for dup() */
static FILE *invlist_fopen(const char *path, const char *mode, long *len)
{
	char invlist_path[MAX_PATH_LEN];
	sprintf(invlist_path, "%s" ".bin", path);

	FILE *fh = fopen(invlist_path, mode);

	if (len != NULL) {
		int fd = fileno(fh);
		FILE *fh_dup = fdopen(dup(fd), "r");
		fseek(fh_dup, 0, SEEK_END);
		*len = ftell(fh_dup);
		fclose(fh_dup);
	}

	return fh;
}

int invlist_empty(struct invlist* invlist)
{
	if (invlist->type == INVLIST_TYPE_INMEMO) {
		return (invlist->head == NULL);
	} else {
		long len;
		fclose(invlist_fopen(invlist->path, "r", &len));
		return (len == 0);
	}
}

void invlist_free(struct invlist *invlist)
{
	skippy_free(&invlist->skippy, struct invlist_node, sn,
		free(p->blk);
		free(p)
	);
	free(invlist);
}

/*
 * Iterator/Writer creation/destroy functions
 */

static size_t refill_buffer__memo(struct invlist_iterator *iter)
{
	struct invlist_node *cur = iter->cur;
	size_t out_sz = 0;
	if (cur) {
		codec_buf_struct_info_t *c_info = iter->c_info;
		/* decode and refill buffer */
		out_sz = codec_buf_decode(iter->buf, cur->blk, cur->len, c_info);
		iter->buf_len = cur->len;
	} else {
		iter->buf_len = 0;
	}

	iter->buf_idx = 0;
	return out_sz;
}

static size_t refill_buffer__disk(struct invlist_iterator *iter, long offset)
{
	fseek(iter->lfh, offset, SEEK_SET);

	size_t rd_sz, out_sz;
	struct _head {
		uint16_t len;
		uint16_t size;
	} head;

	/* reset buffer in case of error */
	iter->buf_idx = 0;
	iter->buf_len = 0;

	rd_sz = fread(&head, 1, sizeof head, iter->lfh);

#ifdef INVLIST_DEBUG
	printf("seek to %lu, read [len=%u, sz=%u + %lu]\n", offset,
		head.len, head.size, sizeof head);
#endif

	if (rd_sz != sizeof head) {
		prerr("refill_buffer__disk head read: %u", rd_sz);
		return 0;
	}

	char block[head.size]; /* allocate read buffer */
	rd_sz = fread(block, 1, head.size, iter->lfh);
	if (rd_sz != head.size) {
		prerr("refill_buffer__disk block read: %u", rd_sz);
		return 0;
	}

	codec_buf_struct_info_t *c_info = iter->c_info;
	/* decode and refill buffer */
	out_sz = codec_buf_decode(iter->buf, block, head.len, c_info);
	iter->buf_len = head.len;
	return out_sz;
}

uint64_t invlist_iter_bufkey(struct invlist_iterator* iter, uint32_t idx)
{
	/* wrap bufkey callback function with a termination check */
	if (iter->type == INVLIST_TYPE_ONDISK) {
		/* return max ID if iterator terminated on-disk */
		if (skippy_fend(&iter->sfh)) return UINT64_MAX;
	} else {
		/* return max ID if iterator terminated in-memory */
		if (iter->cur == NULL) return UINT64_MAX;
	}

	return (*iter->bufkey)(iter, idx);
}

uint64_t
invlist_iter_default_bufkey(struct invlist_iterator* iter, uint32_t idx)
{
	uint32_t key;
	codec_buf_struct_info_t *c_info = iter->c_info;
	CODEC_BUF_GET(key, iter->buf, 0, idx, c_info);

	return key;
}

static invlist_iter_t base_iterator(struct invlist *invlist)
{
	invlist_iter_t iter = malloc(sizeof *iter);

	iter->buf = NULL;
	iter->buf_idx = 0;
	iter->buf_len = 0;
	iter->invlist = invlist;
	iter->type = invlist->type;
	iter->path = strdup(invlist->path);
	iter->buf_max_len = invlist->buf_max_len;
	iter->buf_max_sz  = invlist->buf_max_sz;

	iter->c_info = invlist->c_info;
	iter->bufkey = invlist->bufkey;

	iter->cur = invlist->head;
	iter->lfh = NULL;

	return iter;
}

/* an inverted list writer */
invlist_iter_t invlist_writer(struct invlist *invlist)
{
	invlist_iter_t iter = base_iterator(invlist);

	if (invlist->type == INVLIST_TYPE_INMEMO)
		iter->buf = codec_buf_alloc(invlist->buf_max_len, invlist->c_info);

	return iter;
}

/* an inverted list reader */
invlist_iter_t invlist_iterator(struct invlist *invlist)
{
	invlist_iter_t iter = base_iterator(invlist);
	iter->buf = codec_buf_alloc(invlist->buf_max_len, invlist->c_info);

	if (invlist->type == INVLIST_TYPE_INMEMO) {
		/* a in-memo invlist */
		assert(iter->cur != NULL);
		refill_buffer__memo(iter);
	} else {
		/* a on-disk invlist */
		iter->lfh = invlist_fopen(iter->path, "r", NULL);
		assert(iter->lfh != NULL);

		int error = skippy_fopen(&iter->sfh, iter->path, "r",
			iter->buf_max_len /* skippy span the same as buffer length */);
		assert(!error); (void)error;
		(void)skippy_fnext(&iter->sfh, 0);

		refill_buffer__disk(iter, 0);
	}

	return iter;
}

void invlist_iter_free(struct invlist_iterator *iter)
{
	if (iter->lfh) {
		/* this is a reader */
		skippy_fclose(&iter->sfh);
		fclose(iter->lfh);
	}

	if (iter->buf)
		codec_buf_free(iter->buf, iter->c_info);

	free(iter->path);
	free(iter);
}

/*
 * writer / flush functions
 */

static struct invlist_node
*create_node(uint64_t key, uint16_t size, uint16_t len)
{
	struct invlist_node *ret = malloc(sizeof *ret);

	skippy_node_init(&ret->sn, key);
	ret->blk = malloc(size);
	ret->len = len;
	ret->size = size;

	return ret;
}

static void append_node(struct invlist *invlist,
	struct invlist_node *node, size_t node_sz)
{
	if (invlist->head == NULL)
		invlist->head = node;

	skippy_append(&invlist->skippy, &node->sn);
}

static struct skippy_data
ondisk_invlist_block_writer(struct skippy_node *blk_, void *args_)
{
	struct skippy_data sd;
	struct invlist_node *node;

	/* get node pointer address */
	node = container_of(blk_, struct invlist_node, sn);
	sd.key = node->sn.key; /* return key */

	/* get other arguments */
	PTR_CAST(fh, FILE, args_);

	/* record the current file offset */
	fseek(fh, 0, SEEK_END);
	sd.child_offset = ftell(fh);

#ifdef INVLIST_DEBUG
	printf("seek to %lu, write [head=%u + size=%lu]\n", offset,
		sizeof node->size, node->size);
#endif

	/* write the invlist_node onto disk */
	fwrite(&node->size, 1, sizeof node->size, fh);
	fwrite(node->blk, 1, node->size, fh);

	return sd;
}

size_t __invlist_writer_flush(struct invlist_iterator *iter)
{
	size_t payload_sz;
	struct invlist *invlist = iter->invlist;

	if (iter->buf_len == 0)
		return 0;

	/* encode current buffer integers */
	char enc_buf[iter->buf_max_sz];
	payload_sz = codec_buf_encode(enc_buf,
		iter->buf, iter->buf_len, iter->c_info);

	/* get the key for this flushed block */
	uint64_t key = invlist_iter_bufkey(iter, 0);

	/* do flushing */
	if (iter->type == INVLIST_TYPE_INMEMO) {
		/* append new in-memory block with encoded buffer */
		struct invlist *invlist = iter->invlist;
		struct invlist_node *node = create_node(key, payload_sz, iter->buf_len);
		memcpy(node->blk, enc_buf, payload_sz);
		append_node(invlist, node, sizeof *node + payload_sz);
		iter->cur = node;

		invlist->tot_payload_sz += sizeof *node;
	} else {
		/* write new on-disk block with encoded buffer */
		struct invlist_node *node = create_node(key, payload_sz, iter->buf_len);
		memcpy(node->blk, enc_buf, payload_sz);

		/* write on-disk inverted list and skip structure */
		struct skippy_fh sfh;
		FILE *lfh = invlist_fopen(iter->path, "a", NULL);
		assert(NULL != lfh);
		int error = skippy_fopen(&sfh, iter->path, "a", iter->buf_max_len);
		assert(!error); (void)error;
		skippy_fwrite(&sfh, &node->sn, ondisk_invlist_block_writer, lfh);
		skippy_fclose(&sfh);
		fclose(lfh);

		/* free temporary node */
		free(node->blk);
		free(node);

		invlist->tot_payload_sz += sizeof node->size;
	}

	/* reset iterator pointers/buffer */
	iter->buf_idx = 0;
	iter->buf_len = 0;

	/* update inverted list sizes */
	invlist->tot_payload_sz += payload_sz;
	invlist->n_blk ++;
	return payload_sz;
}

size_t invlist_writer_flush(struct invlist_iterator *iter)
{
	size_t flush_sz = 0;
	if (iter->type == INVLIST_TYPE_INMEMO) {
		/* in memory */

		flush_sz = __invlist_writer_flush(iter);
	} else {
		/* on disk */

		/* make path string */
		char path[MAX_PATH_LEN];
		sprintf(path, "%s-%s.bin", iter->path, INVLIST_DISK_CODECBUF_NAME);

		/* allocate file descriptor */
		int fd = open(path, O_CREAT | O_RDONLY, 0666);
		assert(fd >= 0);

		/* create a temporary codec buffer here */
		iter->buf = codec_buf_alloc(iter->buf_max_len, iter->c_info);

		/* read on-disk codec buffer into temporary codec buffer */
		const size_t rd_sz = iter->c_info->struct_sz;
		char item[rd_sz];
		int cnt = 0;
		while (rd_sz == read(fd, item, rd_sz)) {
			codec_buf_set(iter->buf, cnt, item, iter->c_info);
			cnt ++;
		}
		close(fd);

		/* do flush (to be compressed) */
		flush_sz = __invlist_writer_flush(iter);

		/* destory temporary codec buffer */
		codec_buf_free(iter->buf, iter->c_info);
		iter->buf = NULL;
	}

	return flush_sz;
}

size_t invlist_writer_write(struct invlist_iterator *iter, const void *in)
{
	size_t flush_sz = 0;

	/* flush buffer on inefficient buffer space */
	if (iter->buf_idx + 1 > iter->buf_max_len)
		flush_sz = invlist_writer_flush(iter);

	/* append to buffer */
	if (iter->type == INVLIST_TYPE_INMEMO) {
		/* in memory */
		codec_buf_set(iter->buf, iter->buf_idx, (void*)in, iter->c_info);
		iter->buf_idx ++;
	} else {
		/* write against disk to save memory here */
		char path[MAX_PATH_LEN];
		sprintf(path, "%s-%s.bin", iter->path, INVLIST_DISK_CODECBUF_NAME);

		int fd;
		if (iter->buf_idx == 0)
			fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		else
			fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0666);
		assert(fd >= 0);
		ssize_t wr_sz = write(fd, in, iter->c_info->struct_sz);
		(void)wr_sz;
		assert(wr_sz == iter->c_info->struct_sz);

		iter->buf_idx = lseek(fd, 0, SEEK_END) / iter->c_info->struct_sz;
		close(fd);
	}

	iter->buf_len = iter->buf_idx;

	return flush_sz;
}

/*
 * Iterator core functions
 */

size_t invlist_iter_read(struct invlist_iterator* iter, void *dest)
{
	uint32_t idx = iter->buf_idx;
	size_t out_sz = 0;
	codec_buf_struct_info_t *c_info = iter->c_info;

	for (int j = 0; j < c_info->n_fields; j++) {
		void *field_dest = (char *)dest + c_info->field_info[j].offset;
		CODEC_BUF_CPY(field_dest, iter->buf, j, idx, c_info);
		out_sz += c_info->field_info[j].sz;
	}

	return out_sz;
}

static struct invlist_node *next_blk(struct invlist_node *cur)
{
	struct skippy_node *next = cur->sn.next[0];
	return container_of(next, struct invlist_node, sn);
}

int invlist_iter_next(struct invlist_iterator* iter)
{
	if (iter->buf_idx + 1 < iter->buf_len) {
		iter->buf_idx += 1;
		return 1;

	} else if (iter->type == INVLIST_TYPE_INMEMO) {
		/* refill buffer from memory */
		if (iter->cur != NULL) {
			iter->cur = next_blk(iter->cur);
			refill_buffer__memo(iter);
		}
		return (iter->cur != NULL);

	} else {
		/* refill buffer from disk */
		struct skippy_data sd = skippy_fnext(&iter->sfh, 0);
		if (sd.key == 0) return 0;

		refill_buffer__disk(iter, sd.child_offset);
		return 1;
	}

	return 0;
}

int invlist_iter_jump(struct invlist_iterator* iter, uint64_t target)
{
	struct skippy_node *jump_to;
	uint64_t            id;

	/* safe guard for buf_len */
	if (iter->buf_len == 0) {
#ifdef INVLIST_DEBUG
		printf("buf_len == 0 when skipping\n");
#endif
		goto step;
	}

	/* if buffer block-end ID is greater, no need to jump */
	id = invlist_iter_bufkey(iter, iter->buf_len - 1);
	if (target <= id) {
#ifdef INVLIST_DEBUG
		printf("target <= block-end id, no need to jump\n");
#endif
		goto step;
	}

	/* jump according to iterator type */
	if (iter->type == INVLIST_TYPE_INMEMO) {
		/* in-memory jump */
		if (iter->cur == NULL) { return 0; }

		/* perform skipping */
		jump_to = skippy_node_lazy_jump(&iter->cur->sn, target);

		/* this is a new block, update pointer and refill buffer. */
		iter->cur = container_of(jump_to, struct invlist_node, sn);
		refill_buffer__memo(iter);

	} else {
		/* on-disk jump */
		struct skippy_data sd;
		sd = skippy_fskip(&iter->sfh, target);

		/* refill buffer from disk data after skipping. */
		refill_buffer__disk(iter, sd.child_offset);
	}

step: /* step-by-step advance */
	do {
		id = invlist_iter_bufkey(iter, iter->buf_idx);
#ifdef INVLIST_DEBUG
		printf("step-by-step key: %lu\n", id);
#endif
		if (id >= target) return 1;

	} while (invlist_iter_next(iter));

	return 0;
}

/*
 * test function
 */

void invlist_iter_print_as_decoded_ints(invlist_iter_t iter)
{
	codec_buf_struct_info_t *c_info = iter->c_info;

	/* print table header */
	printf("[%8s%4s%8s]: ", "", "key", "");
	for (int j = 0; j < c_info->n_fields; j++) {
		const char *name = c_info->field_info[j].name;
		printf("%6.6s ", name);
	}
	printf("\n");

	int cnt = 0;
	do {
		uint64_t key = invlist_iter_bufkey(iter, iter->buf_idx);
		uint32_t idx = iter->buf_idx;
		printf("[%20lu]: ", key);

		for (int j = 0; j < c_info->n_fields; j++) {
			void *addr = CODEC_BUF_ADDR(iter->buf, j, idx, c_info);

			switch (c_info->field_info[j].sz) {
			case 1:
				printf("%6u ", *(uint8_t*)addr);
				break;
			case 2:
				printf("%6u ", *(uint16_t*)addr);
				break;
			case 4:
				printf("%6u ", *(uint32_t*)addr);
				break;
			default:
				printf("error! ");
				break;
			}
		}

		cnt ++;
		printf("\n");

	} while (invlist_iter_next(iter));
}

void invlist_print_as_decoded_ints(struct invlist* invlist)
{
	invlist_iter_t iter = invlist_iterator(invlist);
	invlist_iter_print_as_decoded_ints(iter);
	invlist_iter_free(iter);
}
