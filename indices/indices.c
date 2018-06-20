#include "term-index/config.h"
#include "indices.h"
#include "config.h"

void indices_init(struct indices* indices)
{
	indices->ti = NULL;
	indices->mi = NULL;
	indices->url_bi = NULL;
	indices->txt_bi = NULL;
}

bool indices_open(struct indices* indices, const char* index_path,
                  enum indices_open_mode mode)
{
	/* return variables */
	bool                  open_err = 0;

	/* temporary variables */
	const char            blob_index_url_name[] = "url";
	const char            blob_index_txt_name[] = "doc";
	char                  path[MAX_FILE_NAME_LEN];

	/* indices variables */
	void                 *term_index = NULL;
	math_index_t          math_index = NULL;
	blob_index_t          blob_index_url = NULL;
	blob_index_t          blob_index_txt = NULL;

	/*
	 * open term index.
	 */
	sprintf(path, "%s/term", index_path);

	if (mode == INDICES_OPEN_RW)
		mkdir_p(path);

	term_index = term_index_open(path, (mode == INDICES_OPEN_RD) ?
	                             TERM_INDEX_OPEN_EXISTS:
	                             TERM_INDEX_OPEN_CREATE);
#ifndef IGNORE_TERM_INDEX
	if (NULL == term_index) {
		fprintf(stderr, "cannot create/open term index.\n");
		open_err = 1;

		goto skip;
	}
#endif

	/*
	 * open math index.
	 */
	math_index = math_index_open(index_path, (mode == INDICES_OPEN_RD) ?
	                             MATH_INDEX_READ_ONLY: MATH_INDEX_WRITE);
	if (NULL == math_index) {
		fprintf(stderr, "cannot create/open math index.\n");

		open_err = 1;
		goto skip;
	}

	/*
	 * open blob index
	 */
	sprintf(path, "%s/%s", index_path, blob_index_url_name);
	blob_index_url = blob_index_open(path, (mode == INDICES_OPEN_RD) ?
	                                 BLOB_OPEN_RD : BLOB_OPEN_WR);
	if (NULL == blob_index_url) {
		fprintf(stderr, "cannot create/open URL blob index.\n");

		open_err = 1;
		goto skip;
	}

	sprintf(path, "%s/%s", index_path, blob_index_txt_name);
	blob_index_txt = blob_index_open(path, (mode == INDICES_OPEN_RD) ?
	                                 BLOB_OPEN_RD : BLOB_OPEN_WR);
	if (NULL == blob_index_txt) {
		fprintf(stderr, "cannot create/open text blob index.\n");

		open_err = 1;
		goto skip;
	}

skip:
	indices->ti = term_index;
	indices->mi = math_index;
	indices->url_bi = blob_index_url;
	indices->txt_bi = blob_index_txt;

	return open_err;
}

void indices_close(struct indices* indices)
{
	if (indices->ti) {
		term_index_close(indices->ti);
		indices->ti = NULL;
	}

	if (indices->mi) {
		math_index_close(indices->mi);
		indices->mi = NULL;
	}

	if (indices->url_bi) {
		blob_index_close(indices->url_bi);
		indices->url_bi = NULL;
	}

	if (indices->txt_bi) {
		blob_index_close(indices->txt_bi);
		indices->txt_bi = NULL;
	}
}

