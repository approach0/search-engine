#include <stdio.h>  /* for printf() */
#include <stdlib.h> /* for free() */
#include <stdint.h>

#include "blob-index.h"
#include "config.h"

#ifdef DEBUG_BLOBINDEX
#undef N_DEBUG
#else
#define N_DEBUG
#endif

#include <assert.h>

#if 0
typedef off_t blob_ptr_t; /* large index */
#else
typedef uint32_t blob_ptr_t; /* small index */
#endif

typedef uint32_t blob_sz_t;

struct blob_index {
	FILE *ptr_file;
	FILE *dat_file;
};

blob_index_t
blob_index_open(const char * path, enum blob_open_mode mode)
{
	struct blob_index *bi;
	char ptr_file_path[BLOBINDEX_FILEPATH_MAX_LEN];
	char dat_file_path[BLOBINDEX_FILEPATH_MAX_LEN];

	sprintf(ptr_file_path, "%s.ptr.bin", path);
	sprintf(dat_file_path, "%s.dat.bin", path);

reopen:
	bi = calloc(1, sizeof(struct blob_index));
	assert(NULL != bi);

	if (mode == BLOB_OPEN_RD) {
		bi->ptr_file = fopen(ptr_file_path, "r");
		bi->dat_file = fopen(dat_file_path, "r");
	} else if (mode == BLOB_OPEN_WR) {
		bi->ptr_file = fopen(ptr_file_path, "r+");
		/*
		 * file at ptr_file_path does not exists, since
		 * "r+" mode does not create a file, we need to
		 * create it ourself.
		 */
		if (bi->ptr_file == NULL) {
			bi->ptr_file = fopen(ptr_file_path, "w");
			if (bi->ptr_file) {
				blob_index_close(bi);
				goto reopen;
			}
		}

		bi->dat_file = fopen(dat_file_path, "a");
	} else if (mode == BLOB_OPEN_RMA_RANDOM) {
		bi->ptr_file = fopen(ptr_file_path, "r+");
		if (bi->ptr_file == NULL) {
			bi->ptr_file = fopen(ptr_file_path, "w");
			if (bi->ptr_file) {
				blob_index_close(bi);
				goto reopen;
			}
		}

		bi->dat_file = fopen(dat_file_path, "r+");
		if (bi->dat_file == NULL) {
			bi->dat_file = fopen(dat_file_path, "w");
			if (bi->dat_file) {
				blob_index_close(bi);
				goto reopen;
			}
		}
	}

	if (bi->ptr_file == NULL ||
	    bi->dat_file == NULL) {

		blob_index_close(bi);
		return NULL;
	}

	return bi;
}

static blob_ptr_t file_seek_end(FILE *fh)
{
	assert(0 == fseek(fh, 0L, SEEK_END));
	return (blob_ptr_t)ftell(fh);
}

size_t
blob_index_write(blob_index_t index, doc_id_t docID,
                 const void *blob, size_t size)
{
	struct blob_index *bi = (struct blob_index *)index;
	blob_ptr_t ptr_wr_pos = (docID * sizeof(blob_ptr_t));
	blob_ptr_t dat_pos = ftell(bi->dat_file);
	blob_sz_t  blob_sz;
	size_t blob_sz_written;

	assert(0 == fseek(bi->ptr_file, ptr_wr_pos, SEEK_SET));

#ifdef DEBUG_BLOBINDEX
	printf("blob writing: ptr_wr_pos, dat_pos @ (%lu, %lu)\n",
	       (off_t)ftell(bi->ptr_file), (off_t)dat_pos);
#endif

	/* write blob data file */
	blob_sz = (blob_sz_t)size;
	fwrite(&blob_sz, 1, sizeof(blob_sz_t), bi->dat_file);
	blob_sz_written = fwrite(blob, 1, size, bi->dat_file);

	/* write pointer file with saved blob data position */
	fwrite(&dat_pos, 1, sizeof(blob_ptr_t), bi->ptr_file);

	return blob_sz_written;
}

size_t
blob_index_replace(blob_index_t index, doc_id_t docID,
                   const void *blob)
{
	struct blob_index *bi = (struct blob_index *)index;
	blob_ptr_t ptr_rd_pos = (docID * sizeof(blob_ptr_t));
	blob_ptr_t dat_pos;
	blob_sz_t  blob_sz;
	size_t blob_sz_written;

	/* read pointer file at ptr_rd_pos position */
	assert(0 == fseek(bi->ptr_file, ptr_rd_pos, SEEK_SET));
	(void)fread(&dat_pos, 1, sizeof(blob_ptr_t), bi->ptr_file);

	/* replace blob data file */
	assert(0 == fseek(bi->dat_file, dat_pos, SEEK_SET));
	(void)fread(&blob_sz, 1, sizeof(blob_sz_t), bi->dat_file);
	blob_sz_written = fwrite(blob, 1, blob_sz, bi->dat_file);

	return blob_sz_written;
}

size_t
blob_index_append(blob_index_t index, doc_id_t docID,
                  const void *blob, size_t size)
{
	struct blob_index *bi = (struct blob_index *)index;
	file_seek_end(bi->dat_file);
	return blob_index_write(index, docID, blob, size);
}

size_t blob_index_read(blob_index_t index, doc_id_t docID, void **blob)
{
	struct blob_index *bi = (struct blob_index *)index;
	blob_ptr_t ptr_rd_pos = (docID * sizeof(blob_ptr_t));
	blob_ptr_t ptr_file_end = file_seek_end(bi->ptr_file);
	blob_ptr_t dat_pos;
	blob_sz_t  blob_sz;

	if (ptr_rd_pos + sizeof(blob_ptr_t) > ptr_file_end) {
		fprintf(stderr, "blob index: not indexed docID: %u.\n", docID);
		*blob = NULL;
		return 0;
	}

#ifdef DEBUG_BLOBINDEX
	printf("blob reading: ptr_wr_pos @ %lu.\n", (off_t)ptr_rd_pos);
#endif
	/* seek pointer file to get blob data position */
	assert(0 == fseek(bi->ptr_file, ptr_rd_pos, SEEK_SET));
	(void)fread(&dat_pos, 1, sizeof(blob_ptr_t), bi->ptr_file);

#ifdef DEBUG_BLOBINDEX
	printf("blob reading: dat_pos @ %lu.\n", (off_t)dat_pos);
#endif
	/* seek to that data position and first get blob size */
	assert(0 == fseek(bi->dat_file, dat_pos, SEEK_SET));
	(void)fread(&blob_sz, 1, sizeof(blob_sz_t), bi->dat_file);

	/* alloc read buffer */
	*blob = malloc(blob_sz);

	/* then, get blob binary */
	return fread(*blob, 1, blob_sz, bi->dat_file);
}

void blob_free(void *blob)
{
	free(blob);
}

void blob_index_close(blob_index_t index)
{
	struct blob_index *bi = (struct blob_index *)index;

	if (bi->ptr_file)
		fclose(bi->ptr_file);

	if (bi->dat_file)
		fclose(bi->dat_file);

	free(index);
	return;
}
