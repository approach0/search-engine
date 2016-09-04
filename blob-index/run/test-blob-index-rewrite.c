#include "mhook/mhook.h"
#include "blob-index.h"
#include <string.h> /* for strlen() */
#include <stdio.h>  /* for printf() */
#include <time.h>
#include <stdlib.h>

static void print(char *p, size_t sz)
{
	int i;
	for (i = 0; i < sz; i++)
		printf("%c", p[i]);
	printf("\n");
}

int main()
{
	const char string1[] = "hello";
	const char string2[] = "world!";
	const char string3[] = "blob";
	const char string4[] = "index";
	char *out;
	size_t sz;
	blob_index_t bi;

	/* write in two opens */
	bi = blob_index_open("./test", BLOB_OPEN_WR);
	blob_index_write(bi, 1, string1, strlen(string1));
	blob_index_write(bi, 2, string2, strlen(string2));
	blob_index_close(bi);

	bi = blob_index_open("./test", BLOB_OPEN_WR);
	blob_index_write(bi, 3, string3, strlen(string3));
	blob_index_write(bi, 4, string4, strlen(string4));
	blob_index_close(bi);

	/* read and check */
	bi = blob_index_open("./test", BLOB_OPEN_RD);
	sz = blob_index_read(bi, 1, (void**)&out);
	print(out, sz);
	free(out);

	sz = blob_index_read(bi, 2, (void**)&out);
	print(out, sz);
	free(out);

	sz = blob_index_read(bi, 3, (void**)&out);
	print(out, sz);
	free(out);

	sz = blob_index_read(bi, 4, (void**)&out);
	print(out, sz);
	free(out);

	blob_index_read(bi, 5, (void**)&out);

	blob_index_close(bi);

	/* check unfree */
	mhook_print_unfree();
	return 0;
}
