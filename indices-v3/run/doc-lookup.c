#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "mhook/mhook.h"
#include "common/common.h"
#include "codec/codec.h"
#include "indices.h"
#include "term-index/print-utils.h"

static void print_document(struct indices *indices, doc_id_t docID)
{
	uint32_t docLen = term_index_get_docLen(indices->ti, docID);
	printf("doc#%u length: %u\n", docID, docLen);

	char *text;
	size_t len;

	text = get_blob_txt(indices->url_bi, docID, 0, &len);
	printf("%s\n\n", text);
	free(text);

	text = get_blob_txt(indices->txt_bi, docID, 1, &len);
	printf("%s\n", text);
	free(text);
}

static void print_term(term_index_t ti, char *term)
{
	term_id_t term_id = term_lookup(ti, term);
	printf("term: `%s'\n", term);

	if (term_id == 0) {
		printf("non-existing term.\n");
		return;
	}

	uint32_t df = term_index_get_df(ti, term_id);
	printf("df = %u\n", df);

	struct term_invlist_entry_reader entry_reader;
	entry_reader = term_index_lookup(ti, term_id);

	if (entry_reader.inmemo_reader)
		print_inmemo_term_items(entry_reader.inmemo_reader);
	if (entry_reader.ondisk_reader)
		print_ondisk_term_items(entry_reader.ondisk_reader);
}

static void print_math(math_index_t index, char *path_key)
{
	struct math_invlist_entry_reader entry_reader;
	printf("token path: %s\n", path_key);

	entry_reader = math_index_lookup(index, path_key);
	if (entry_reader.pf) {
		printf("pf = %u, type = %s\n", entry_reader.pf,
			(entry_reader.medium == MATH_READER_MEDIUM_INMEMO) ?
			"in-memory" : "on-disk");
		invlist_iter_print_as_decoded_ints(entry_reader.reader);
		invlist_iter_free(entry_reader.reader);
		fclose(entry_reader.fh_symbinfo);
	} else {
		printf("non-existing token path.\n");
	}
}

int main(int argc, char* argv[])
{
	struct indices indices;
	char  *index_path = NULL;

	doc_id_t docID = 0;
	int opt;

	char term[64] = "";
	char path[MAX_DIR_PATH_NAME_LEN] = "";

	uint32_t math_cache_sz = 15 KB;
	uint32_t term_cache_sz = 15 KB;

	while ((opt = getopt(argc, argv, "hp:d:t:m:c:C:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("Lookup document blob strings. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s \n"
			       " -h (show help text) \n"
			       " -p <index path> \n"
			       " -d <docID> (show document) \n"
			       " -t <term> (show term inverted list) \n"
			       " -m <token_path> "
			       "(show math inverted list, e.g. /prefix/VAR/BASE/HANGER) \n"
			       " -c <KB> (specify term cache size) \n"
			       " -C <KB> (specify math cache size) \n"
			       "\n", argv[0]);
			goto exit;

		case 'p':
			index_path = strdup(optarg);
			break;

		case 'd':
			sscanf(optarg, "%u", &docID);
			break;

		case 't':
			sscanf(optarg, "%s", term);
			break;

		case 'm':
			sscanf(optarg, "%s", path);
			break;

		case 'c':
			sscanf(optarg, "%u", &term_cache_sz);
			term_cache_sz = term_cache_sz KB;
			break;

		case 'C':
			sscanf(optarg, "%u", &math_cache_sz);
			math_cache_sz = math_cache_sz KB;
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (index_path) {
		printf("index path: %s\n", index_path);
	} else {
		printf("no index path specified.\n");
		goto exit;
	}

	if(indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		fprintf(stderr, "indices open failed.\n");
		goto close;
	}

	/* caching some into memory */
	indices.mi_cache_limit = math_cache_sz;
	indices.ti_cache_limit = term_cache_sz;
	printf("caching (term-index %u KB, math-index %u KB)\n",
		term_cache_sz / (1 KB), math_cache_sz / (1 KB));
	printf("\n");
	indices_cache(&indices);

	/* print summary anyway */
	printf("\n");
	indices_print_summary(&indices);
	printf("\n");

	/* print document content */
	if (docID != 0)
		print_document(&indices, docID);

	/* print term inverted list */
	if (term[0] != '\0')
		print_term(indices.ti, term);

	/* print term inverted list */
	if (path[0] != '\0')
		print_math(indices.mi, path);

close:
	indices_close(&indices);

exit:
	free(index_path);

	mhook_print_unfree();
	return 0;
}
