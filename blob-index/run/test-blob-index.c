#include "blob-index.h"
#include <string.h> /* for strlen() */
#include <stdio.h>  /* for printf() */

int main()
{
	size_t n_read;
	const char  blob_in[] = "abc";
	char       *blob_out = NULL;

	blob_index_t bi = blob_index_open("./somewhere");

	blob_index_write(bi, 123, blob_in, strlen(blob_in));
	n_read = blob_index_read(bi, 123, (void **)&blob_out);
	printf("blob_out length = %lu\n", n_read);

	if (blob_out != NULL)
		blob_free(blob_out);

	blob_index_close(bi);
}
