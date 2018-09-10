#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>

#include "mhook/mhook.h"
#include "math-index.h"
#include "config.h" /* for TOKEN_PATH_NAME */

int math_posting_iter_test(const char* path)
{
	math_posting_t iter1 = math_posting_iterator(path);
	math_posting_t iter2 = NULL;

	if (iter1 == NULL)
		return 1;

	do {
		uint64_t cur = math_posting_iter_cur(iter1);
		uint32_t docID = (uint32_t)(cur >> 32);
		uint32_t expID = (uint32_t)(cur >> 0);
		printf("iter1: %u, %u \n", docID, expID);

		if (iter2) {
			cur = math_posting_iter_cur(iter2);
			docID = (uint32_t)(cur >> 32);
			expID = (uint32_t)(cur >> 0);
			printf("iter2: %u, %u \n", docID, expID);

			if (random() % 2) {
				printf("iter2 advances ...\n");
				math_posting_iter_next(iter2);
			}
		}

		if (docID >= 30 && iter2 == NULL)
			iter2 = math_posting_copy(iter1);

		printf("iter1 advances ...\n");
		printf("\n");
	} while (math_posting_iter_next(iter1));

	if (iter2)
		math_posting_iter_free(iter2);
	math_posting_iter_free(iter1);
	return 0;
}

int main(int argc, char *argv[])
{
	int opt;
	uint32_t find_docID = 0;
	const char *path = NULL;
	while ((opt = getopt(argc, argv, "htp:d:")) != -1) {
		switch (opt) {
		case 'h':
			printf("USAGE:\n");
			printf("%s -h |"
			       " -d (docID to lookup) |"
			       " -p <path contains .bin files>"
			       , argv[0]);
			printf("\n");
			goto exit;

		case 'p':
			path = strdup(optarg);
			break;

		case 'd':
			sscanf(optarg, "%u", &find_docID);
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (NULL == path) {
		printf("no path specified.\n");
		goto exit;
	}

	// math_inex_probe_v2(path, 1, stdout);

	if (find_docID) {
		printf("finding doc#%u ...\n", find_docID);
		foreach (iter, math_posting, path) {
			uint64_t cur = math_posting_iter_cur(iter);
			uint32_t docID = (uint32_t)(cur >> 32);
			uint32_t expID = (uint32_t)(cur >> 0);

			if (docID >= find_docID + 10)
				break;
			//if (docID == find_docID)
				printf("%u, %u \n", docID, expID);
		}
	} else {
		printf("iterator test ...\n");
		math_posting_iter_test(path);
	}

	free((void*)path);
exit:
	mhook_print_unfree();
	return 0;
}
