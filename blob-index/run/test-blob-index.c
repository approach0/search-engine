#include "mhook/mhook.h"
#include "blob-index.h"
#include <string.h> /* for strlen() */
#include <stdio.h>  /* for printf() */
#include <time.h>
#include <stdlib.h>

int main()
{
	size_t   size;
	char     blob_in[10];
	size_t   i, j, rand_sz;
	char    *blob_out = NULL;
	doc_id_t docID;

	/* write */
	srand(time(NULL));
	blob_index_t bi = blob_index_open("./test", BLOB_OPEN_WR);

	for (docID = 0; docID < 100; docID++) {
		rand_sz = rand() % 9 + 1;

		for (i = 0; i < rand_sz; i++)
			blob_in[i] = '0' + (rand_sz - i);

		size = blob_index_write(bi, docID, blob_in, rand_sz);
		printf("written doc#%u: size = %lu\n\n", docID, size);
	}

	blob_index_close(bi);

	/* read */
	bi = blob_index_open("./test", BLOB_OPEN_RD);

	for (i = 0; i < 10; i++) {
		docID = rand() % 150;

		size = blob_index_read(bi, docID, (void **)&blob_out);
		printf("read doc#%u: size = %lu, ", docID, size);
		printf("blob: ");

		if (blob_out != NULL) {
			for (j = 0; j < size; j++)
				printf("[%c]", blob_out[j]);

			blob_free(blob_out);
		} else {
			printf(" NULL");
		}

		printf("\n");
	}

	blob_index_close(bi);

	mhook_print_unfree();
	return 0;
}
