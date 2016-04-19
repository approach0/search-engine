#include <string.h>
#include <stdio.h>
#include "completion.h"

const char *completion_terms[] = {
	/* auto-generated completion terms */
	/* INSERT_HERE */
	"END"
};

void completion_callbk(const char *buf, linenoiseCompletions *lc, size_t pos)
{
	const char **terms = completion_terms;
	const size_t n_terms = sizeof(completion_terms) / sizeof(char*);
	size_t i, j, n, start, termlen;
	char lookahread[1024];
	char lookback[1024];
	char newbuf[1024];

	if (pos == 0)
		return;

	/* Find the start position to compare lookup terms. */
	for (start = pos - 1; start > 0 && buf[start] != '\\'; start --);

	/* For each lookup terms, see if it has prefix matches the input string. */
	for (n = 0; n < n_terms; n++) {
		termlen = strlen(terms[n]);
		for (i = start, j = 0; i < pos && j < termlen; i++, j++) {
			if (buf[i] != terms[n][j])
				break;
		}

		/* If it matches, it is a completion option to add. */
		if (i == pos) {
			/* Setup the `lookback' buffer containing string before the
			 * position where lookup term being inserted. */
			snprintf(lookback, start + 1, "%s", buf);
			/* Setup the `lookahread' buffer containing string after the
			 * position where lookup term being inserted. */
			strcpy(lookahread, buf + pos);
			/* Concatenate everything. */
			sprintf(newbuf, "%s%s%s", lookback, terms[n], lookahread);

			/* Add this completion option. */
			linenoiseAddCompletion(lc, newbuf, start + termlen);
		}
	}
}
