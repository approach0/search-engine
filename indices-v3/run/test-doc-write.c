#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "mhook/mhook.h"
#include "common/common.h"
#include "txt-seg/lex.h"
#include "indices.h"

static int
parser_exception(struct indexer *indexer, const char *tex, char *msg)
{
	printf("[ TeX parser ]\n");
	printf("%s\n", tex);
	prerr("%s\n", msg);
	return 0;
}

static void
test_write_json(struct indexer *indexer, const char *txt_file)
{
	FILE *fh = fopen(txt_file, "r");
	if (fh == NULL) {
		printf("cannot open %s.\n", txt_file);
		return;
	}

	char content[4096];
	fread(content, 1, 4096, fh);
	printf("%s\n", content);

	strcpy(indexer->url_field, txt_file);
	strcpy(indexer->txt_field, content);
	indexer_write_all_fields(indexer);

	fclose(fh);
}

int main(int argc, char* argv[])
{
	struct indices indices;
	char txt_file[1024] = "./test.txt";

	int opt;
	while ((opt = getopt(argc, argv, "f:")) != -1) {
		switch (opt) {
		case 'f':
			strcpy(txt_file, optarg);
			break;

		default:
			printf("bad argument(s). \n");
			return 1;
		}
	}

	printf("open index for writing ...\n");
	if (indices_open(&indices, "./tmp", INDICES_OPEN_RW)) {
		printf("error open indices.\n");
		return 1;
	} else {
		indices_print_summary(&indices);
	}

	struct indexer *indexer;
	indexer = indexer_alloc(&indices, lex_eng_file, parser_exception);

	test_write_json(indexer, txt_file);

	//strcpy(indexer->url_field, "www.foo.com");
	//strcpy(indexer->txt_field,
	//	"If [imath]3x^2\\equiv1\\bmod p[/imath] then 3 is a quadratic residue\n"
	//	"So [imath]3^{-1}[/imath] is a quadratic residue."
	//	"example of bad TeX: [imath]a+(b[/imath]"
	//);
	//indexer_write_all_fields(indexer);

	//strcpy(indexer->url_field, "www.bar.com");
	//strcpy(indexer->txt_field,
	//	"so multiplying by [imath]2x+k[/imath] positive, [imath]x^2+kx<x(2x+k)[/imath]\n"
	//	"[imath]\\frac{x^2+kx}{2x+k}-x=-\\frac{x^2}{2x+k}<0.[/imath]\n\n"
	//);
	//indexer_write_all_fields(indexer);

	//strcpy(indexer->url_field, "www.example.com");
	//strcpy(indexer->txt_field,
	//	"so so so\n\n"
	//);
	//indexer_write_all_fields(indexer);

	/* print summary */

	indexer_free(indexer);

	indices_close(&indices);
	return 0;
}
