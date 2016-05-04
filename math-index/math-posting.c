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
math_posting_new(struct subpath_ele *ele, const char *fullpath)
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

void math_posting_free(math_posting_t po_)
{
	free(po_);
}

void math_posting_start(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;

	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" MATH_POSTING_FNAME, po->fullpath);

	po->fh_posting = fopen(file_path, "r");
	assert(NULL != po->fh_posting);

	po->fh_pathinfo = fopen(file_path, "r");
	assert(NULL != po->fh_pathinfo);
}

void math_posting_finish(math_posting_t po_)
{
	struct _math_posting *po = (struct _math_posting*)po_;

	if (po->fh_posting)
		fclose(po->fh_posting);

	if (po->fh_pathinfo)
		fclose(po->fh_pathinfo);
}
