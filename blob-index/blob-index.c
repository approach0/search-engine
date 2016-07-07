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

typedef uint32_t blob_ptr_t;
typedef uint32_t blob_sz_t;

struct blob_index {
	FILE *ptr_file;
	FILE *dat_file;
};

blob_index_t blob_index_open(const char * path, const char *mode)
{
	struct blob_index *bi;
	char ptr_file_path[BLOBINDEX_FILEPATH_MAX_LEN];
	char dat_file_path[BLOBINDEX_FILEPATH_MAX_LEN];

	sprintf(ptr_file_path, "%s.ptr.bin", path);
	sprintf(dat_file_path, "%s.dat.bin", path);

	bi = malloc(sizeof(struct blob_index));
	assert(NULL != bi);

	bi->ptr_file = fopen(ptr_file_path, mode);
	bi->dat_file = fopen(dat_file_path, mode);

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
	return (blob_ptr_t)ftell(fh);;
}

size_t
blob_index_write(blob_index_t index, doc_id_t docID,
                 const void *blob, size_t size)
{
	struct blob_index *bi = (struct blob_index *)index;
	blob_ptr_t ptr_wr_pos = (docID * sizeof(blob_ptr_t));
	blob_ptr_t ptr_file_end = file_seek_end(bi->ptr_file);
	blob_ptr_t dat_pos;
	blob_sz_t  blob_sz;
	size_t blob_sz_written;

	if (ptr_wr_pos + sizeof(blob_ptr_t) <= ptr_file_end) {
#ifdef DEBUG_BLOBINDEX
		fprintf(stderr, "%lu <= %u, ", ptr_wr_pos + sizeof(blob_ptr_t),
		        ptr_file_end);
		fprintf(stderr, "docID already indexed.\n");
#endif
		return 0;
	}

	/* seek to data file end and save its position */
	dat_pos = file_seek_end(bi->dat_file);

	/* write blob data file */
	blob_sz = (blob_sz_t)size;
	fwrite(&blob_sz, 1, sizeof(blob_sz_t), bi->dat_file);
	blob_sz_written = fwrite(blob, 1, size, bi->dat_file);

	/* write pointer file with saved blob data position */
	assert(0 == fseek(bi->ptr_file, ptr_wr_pos, SEEK_SET));
	fwrite(&dat_pos, 1, sizeof(blob_ptr_t), bi->ptr_file);

	return blob_sz_written;
}

size_t blob_index_read(blob_index_t index, doc_id_t docID, void **blob)
{
	struct blob_index *bi = (struct blob_index *)index;
	blob_ptr_t ptr_rd_pos = (docID * sizeof(blob_ptr_t));
	blob_ptr_t ptr_file_end = file_seek_end(bi->ptr_file);
	blob_ptr_t dat_pos = 123;
	blob_sz_t  blob_sz = 456;

	if (ptr_rd_pos + sizeof(blob_ptr_t) > ptr_file_end) {
#ifdef DEBUG_BLOBINDEX
		fprintf(stderr, "%lu > %u, ", ptr_rd_pos + sizeof(blob_ptr_t),
		        ptr_file_end);
		fprintf(stderr, "not indexed docID.\n");
#endif
		*blob = NULL;
		return 0;
	}

	/* seek pointer file to get blob data position */
	assert(0 == fseek(bi->ptr_file, ptr_rd_pos, SEEK_SET));
	fread(&dat_pos, 1, sizeof(blob_ptr_t), bi->ptr_file);

	/* seek to that data position and first get blob size */
	assert(0 == fseek(bi->dat_file, dat_pos, SEEK_SET));
	fread(&blob_sz, 1, sizeof(blob_sz_t), bi->dat_file);

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
