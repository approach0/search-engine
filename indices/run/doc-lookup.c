#include <stdio.h>
#include "codec/codec.h"
#include "indexer/config.h" // for MAX_CORPUS_FILE_SZ
#include "indices.h"

int main()
{
	struct codec   codec = {CODEC_GZ, NULL};
	struct indices indices;
	const char     index_path[] = "/home/tk/large-index";

	static char text[MAX_CORPUS_FILE_SZ + 1];
	size_t   blob_sz, text_sz;
	char    *blob_out = NULL;
	doc_id_t docID;

	if(indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	printf("Pleas input blob ID:\n");
	scanf("%u", &docID);

	blob_sz = blob_index_read(indices.txt_bi, docID, (void **)&blob_out);
	printf("read blob of doc#%u: size = %lu ... \n", docID, blob_sz);

	if (blob_out) {
		text_sz = codec_decompress(&codec, blob_out, blob_sz,
		                           text, MAX_CORPUS_FILE_SZ);
		text[text_sz] = '\0';
		blob_free(blob_out);

		printf("TEXT:\n");
		printf("%s", text);
	} else {
		fprintf(stderr, "blob read failed.\n");
	}

close:

	indices_close(&indices);
	return 0;
}
