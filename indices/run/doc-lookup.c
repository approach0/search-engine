#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "mhook/mhook.h"
#include "codec/codec.h"
#include "indexer/config.h" // for MAX_CORPUS_FILE_SZ
#include "indices.h"

int main(int argc, char* argv[])
{
	struct codec   codec = {CODEC_GZ, NULL};
	struct indices indices;
	char  *index_path = NULL;

	static char text[MAX_CORPUS_FILE_SZ + 1];
	size_t   blob_sz, text_sz;
	char    *blob_out = NULL;
	doc_id_t docID = 0;
	int opt;

	while ((opt = getopt(argc, argv, "hp:i:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("Lookup document blob strings. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h | -p <index path> -i <docID>\n", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp -i 123 \n", argv[0]);
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 'i':
			printf("%s\n", optarg);
			sscanf(optarg, "%u", &docID);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (index_path) {
		printf("corpus path: %s\n", index_path);
	} else {
		printf("no corpus path specified.\n");
		goto exit;
	}

	if (docID > 0) {
		printf("looking up docID: %u\n", docID);
	} else {
		printf("docID is zero.\n");
		goto exit;
	}

	if(indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	blob_sz = blob_index_read(indices.url_bi, docID, (void **)&blob_out);
	if (blob_out) {
		memcpy(text, blob_out, blob_sz);
		text[blob_sz] = '\0';
		printf("URL: %s\n\n", text);
		blob_free(blob_out);
	} else {
		fprintf(stderr, "URL blob read failed.\n");
	}

	blob_sz = blob_index_read(indices.txt_bi, docID, (void **)&blob_out);

	if (blob_out) {
		text_sz = codec_decompress(&codec, blob_out, blob_sz,
		                           text, MAX_CORPUS_FILE_SZ);
		text[text_sz] = '\0';
		blob_free(blob_out);

		printf("%s", text);
	} else {
		fprintf(stderr, "text blob read failed.\n");
	}

close:
	indices_close(&indices);

exit:
	free(index_path);

	mhook_print_unfree();
	return 0;
}
