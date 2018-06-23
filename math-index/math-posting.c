#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common/common.h"
#include "head.h"

#define MATH_ONDISK_SIGNATURE "mtdskpst"

#define DISK_RD_BUF_BYTES (DISK_BLCK_SIZE * DISK_RD_BLOCKS)

#define DISK_RD_BUF_ITEMS \
	(DISK_RD_BUF_BYTES / sizeof(struct math_posting_item))

#define DISK_RD_BUF_RMNDR \
	(DISK_RD_BUF_BYTES % sizeof(struct math_posting_item))

struct _math_posting {
	uint64_t   signature;
	const char *fullpath;
	FILE  *fh_posting;
	FILE  *fh_pathinfo;

	/* read buffer */
	uint32_t buf_idx, buf_end;
	struct math_posting_item buf[DISK_RD_BUF_ITEMS];
};

static void print_cur_post_buf(struct _math_posting *po)
{
	uint32_t i;
	struct math_posting_item *item;
	printf("buf (idx=%u, end=%u): ", po->buf_idx, po->buf_end);
	for (i = 0; i < po->buf_end; i++) {
		item = po->buf + po->buf_idx;
		printf("[%u]", i);
		if (i == po->buf_idx)
			printf("{doc%u,exp%u} ", item->doc_id, item->exp_id);
		else
			printf("(doc%u,exp%u) ", item->doc_id, item->exp_id);
	}
	printf("\n");
}

math_posting_t
math_posting_new_reader(const char *fullpath)
{
	struct _math_posting *po =
		malloc(sizeof(struct _math_posting));

	memcpy(&po->signature, MATH_ONDISK_SIGNATURE, 8);
	po->fullpath = fullpath;
	po->fh_posting = NULL;
	po->fh_pathinfo = NULL;
	po->buf_idx = 0;
	po->buf_end = 0;

#ifdef DEBUG_MATH_POSTING
	printf("posting item size: %lu bytes.\n",
	       sizeof(struct math_posting_item));
	printf("DISK_RD_BUF_BYTES: %u\n", DISK_RD_BUF_BYTES);
	printf("sizeof(math posting item): %lu\n",
	       sizeof(struct math_posting_item));
#endif

	/* before compression is implemented, our posting item
	 * structure is a factor of DISK_BLCK_SIZE */
	assert(DISK_RD_BUF_RMNDR == 0);

	return po;
}

int math_posting_exits(const char *fullpath)
{
	char file_path[MAX_DIR_PATH_NAME_LEN];
	FILE *fh_post = NULL, *fh_info = NULL;
	int exists = 1;

	sprintf(file_path, "%s/" MATH_POSTING_FNAME, fullpath);
	fh_post = fopen(file_path, "r");

	sprintf(file_path, "%s/" PATH_INFO_FNAME, fullpath);
	fh_info = fopen(file_path, "r");

	if (NULL == fh_post || NULL == fh_info)
		exists = 0;
	
	if (fh_post) fclose(fh_post);
	if (fh_info) fclose(fh_info);

	return exists;
}

const char *math_posting_get_pathstr(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;
	return po->fullpath;
}

int math_posting_signature(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;
	if (po == NULL) return 0;
	const char signature[] = MATH_ONDISK_SIGNATURE;
	return (memcmp(&po->signature, signature, 8) == 0);
}

void math_posting_free_reader(math_posting_t po_)
{
	free(po_);
}

void math_posting_finish(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;

	if (po->fh_posting)
		fclose(po->fh_posting);

	if (po->fh_pathinfo)
		fclose(po->fh_pathinfo);
}

static __inline size_t
rebuf(void *buf, FILE *fh, uint32_t *idx, uint32_t *end)
{
	size_t nread = fread(buf, sizeof(struct math_posting_item),
	                     DISK_RD_BUF_ITEMS, fh);
	*idx = 0;
	*end = nread;
	return nread;
}

bool math_posting_start(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;

	char file_path[MAX_DIR_PATH_NAME_LEN];

	sprintf(file_path, "%s/" MATH_POSTING_FNAME, po->fullpath);
	po->fh_posting = fopen(file_path, "r");

	sprintf(file_path, "%s/" PATH_INFO_FNAME, po->fullpath);
	po->fh_pathinfo = fopen(file_path, "r");

	if (NULL == po->fh_posting || NULL == po->fh_pathinfo)
		return 0;

	/* start function is required to read the first item */
	rebuf(po->buf, po->fh_posting, &po->buf_idx, &po->buf_end);

	return 1;
}

bool math_posting_next(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;

	po->buf_idx ++; /* one step forward */

	do {
		if (po->buf_idx < po->buf_end)
			/* still in the buffer */
			return 1;
		else
			/* need one read to tell whether or not I can go further */
			rebuf(po->buf, po->fh_posting, &po->buf_idx, &po->buf_end);

	} while (po->buf_end != 0);

	/* buf_end equals zero: reached the end. */
	return 0;
}

bool math_posting_jump(math_posting_t po_, uint64_t target)
{
	uint32_t rightmost; /* right most of the buffer */
	uint64_t *id64;
	struct _math_posting *po = (struct _math_posting*)po_;

	do {
		rightmost = po->buf_end - 1;
		id64 = (uint64_t*)(po->buf + rightmost);

		if (po->buf_end == 0) {
			return 0; /* no more to read, failed at finding target */
		} else if (target <= *id64) {
			/* find that target is in this buffer */
			break;
		} else if (po->buf_end != DISK_RD_BUF_ITEMS) {
			/* cannot read again, we have examined entire file. */
			return 0; /* failed at finding target */
		}

		/* read into buffer */
		rebuf(po->buf, po->fh_posting, &po->buf_idx, &po->buf_end);
	} while (1);

	/* go through the buffer to find the target */
	while (po->buf_idx < po->buf_end) {
		id64 = (uint64_t*)(po->buf + po->buf_idx);

		if (*id64 < target)
			po->buf_idx ++; /* step forward */
		else
			break; /* we find an ID >= target */
	}

	return 1;
}

static struct math_pathinfo_pack*
math_posting_pathinfo(math_posting_t po_, uint32_t position)
{
	pathinfo_num_t i;
	struct _math_posting *po = (struct _math_posting*)po_;

#pragma pack(push, 1)
	static struct {
		struct math_pathinfo_pack head;
		struct math_pathinfo      info_arr[MAX_MATH_PATHS];
	} ret;
#pragma pack(pop)

	/* first go the specified file & position */
	if (-1 == fseek(po->fh_pathinfo, position, SEEK_SET))
		return NULL;

	if (1 == fread(&ret.head, sizeof(struct math_pathinfo_pack),
				   1, po->fh_pathinfo)) {
		/* check the sanity of n_paths */
		if (ret.head.n_paths > MAX_MATH_PATHS)
			return NULL;

		/* now, read all the path info items */
		for (i = 0; i < ret.head.n_paths; i++) {
			if (1 != fread(ret.info_arr + i, sizeof(struct math_pathinfo),
				   1, po->fh_pathinfo))
				return NULL;
		}
	} else {
		return NULL;
	}

	return (struct math_pathinfo_pack*)&ret;
}

static int
math_posting_pathinfo_v2(math_posting_t po_, uint32_t position, uint32_t n_paths,
                         struct math_pathinfo_v2 *pathinfo)
{
	struct _math_posting *po = (struct _math_posting*)po_;

	/* first go the specified file & position */
	if (-1 == fseek(po->fh_pathinfo, position, SEEK_SET))
		return 1;

	if (n_paths == fread(pathinfo, sizeof(struct math_pathinfo_v2),
				   n_paths, po->fh_pathinfo)) {
		return 0;
	} else {
		return 1;
	}
}

void *math_posting_cur_item_v1(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;
	struct math_posting_item *item = po->buf + po->buf_idx;
	static struct math_posting_compound_item_v1 ret;

	ret.exp_id     = item->exp_id;
	ret.doc_id     = item->doc_id;

	struct math_pathinfo_pack *pathinfo_pack;
	pathinfo_pack = math_posting_pathinfo(po, item->pathinfo_pos);

	if (pathinfo_pack == NULL)
		goto ret;
	
	ret.n_paths    = pathinfo_pack->n_paths;
	ret.n_lr_paths = pathinfo_pack->n_lr_paths;

	memcpy(ret.pathinfo, pathinfo_pack->pathinfo,
	       sizeof(struct math_pathinfo) * ret.n_paths);
ret:
	return (struct math_posting_item *)&ret;
}

void *math_posting_cur_item_v2(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;
	PTR_CAST(item, struct math_posting_item_v2, po->buf + po->buf_idx);
	static struct math_posting_compound_item_v2 ret;

	ret.exp_id     = item->exp_id;
	ret.doc_id     = item->doc_id;
	ret.n_paths    = item->n_paths;
	ret.n_lr_paths = item->n_lr_paths;

	if (math_posting_pathinfo_v2(po, item->pathinfo_pos, item->n_paths,
	    ret.pathinfo))
		goto ret;
ret:
	return &ret;
}
