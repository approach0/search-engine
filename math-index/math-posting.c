#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "head.h"

struct _math_posting {
	struct subpath_ele *ele;
	const char *fullpath;
	FILE  *fh_posting;
	FILE  *fh_pathinfo;
	struct math_posting_item cur;
};

math_posting_t
math_posting_new_reader(struct subpath_ele *ele, const char *fullpath)
{
	struct _math_posting *po =
		malloc(sizeof(struct _math_posting));

	po->ele = ele;
	po->fullpath = fullpath;

	return po;
}

struct subpath_ele *math_posting_get_ele(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;
	return po->ele;
}

const char *math_posting_get_pathstr(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;
	return po->fullpath;
}

void math_posting_free_reader(math_posting_t po_)
{
	free(po_);
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
	else
		return 1;
}

void math_posting_finish(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;

	if (po->fh_posting)
		fclose(po->fh_posting);

	if (po->fh_pathinfo)
		fclose(po->fh_pathinfo);
}

bool math_posting_jump(math_posting_t po_, uint64_t to)
{
	return 0;
}

bool math_posting_next(math_posting_t po_)
{
	bool ret = 0;
	struct _math_posting *po = (struct _math_posting*)po_;

	if (1 == fread(&po->cur, sizeof(struct math_posting_item),
	               1, po->fh_posting))
		ret = 1; /* successful read */
	else
		/* indicate (most possibly, the end of file) by
		 * assign a zero docID */
		po->cur.doc_id = 0;

	return ret;
}

struct math_posting_item* math_posting_current(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;
	return &po->cur;
}

struct math_pathinfo_pack*
math_posting_pathinfo(math_posting_t po_, uint64_t position)
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

		/* now, read all the path info items */
		for (i = 0; i < ret.head.n_paths; i++) {
			if (1 != fread(ret.info_arr + i, sizeof(struct math_pathinfo),
				   1, po->fh_pathinfo))
				return NULL;
		}
	}

	return (struct math_pathinfo_pack*)&ret;
}
