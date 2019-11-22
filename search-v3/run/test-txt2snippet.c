#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"
#include "txt2snippet.h"

int main(void)
{
	list hi_li = LIST_NULL;

	/* open file */
	FILE *fh = fopen("test-snippet/sample.txt", "r");
	if (fh == NULL) {
		printf("cannot open file.\n");
		return 1;
	}

	/* read file into string */
	ssize_t rd_sz, txt_sz = 0;
	char txt[4096 * 64];
	while (0 < (rd_sz = fread(txt + txt_sz, 1, 10, fh))) {
		txt_sz += rd_sz;
	}
	fclose(fh);

	/* debug */
	// txt[txt_sz] = '\0';
	// printf("%s", txt);
	// return 0;

	/* prepare snippet (positions are in order number) */
	uint32_t positions[] = {64, 693, 766, 773};
	const int n = sizeof positions / sizeof positions[0];

	hi_li = txt2snippet(txt, txt_sz, positions, n);

	/* print snippet in terminal */
	snippet_hi_print(&hi_li);

	/* free resources */
	snippet_free_highlight_list(&hi_li);
	mhook_print_unfree();
	return 0;
}
