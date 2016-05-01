#include <stdio.h>  /* for printf() */
#include <stdlib.h> /* for free() */
#include "blob-index.h"

blob_index_t blob_index_open(const char * path)
{
	printf("hello blob index!\n");
	return NULL;
}

size_t blob_index_write(blob_index_t index, doc_id_t docID, const void *blob, size_t size)
{
	return 0;
}

size_t blob_index_read(blob_index_t index, doc_id_t docID, void **blob)
{
	return 0;
}

void blob_free(void *blob)
{
	free(blob);
}

void blob_index_close(blob_index_t index)
{
	return;
}
