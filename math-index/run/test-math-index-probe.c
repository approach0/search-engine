#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>

#include "math-index.h"
#include "config.h" /* for TOKEN_PATH_NAME */

int main(int argc, char *argv[])
{
	int opt;
	char *path = NULL;
	bool trans = 0;

	while ((opt = getopt(argc, argv, "htp:")) != -1) {
		switch (opt) {
		case 'h':
			printf("DESCRIPTION:\n");
			printf("probe math index posting list"
			       " given a specified path. \n");
			printf("\n");
			printf("USAGE:\n");
			printf("%s -h |"
			       " -p <path contains .bin files> |"
			       " -t", argv[0]);
			printf("\n");
			printf("EXAMPLE:\n");
			printf("%s -p ./tmp/%s/VAR/TIMES/ADD\n",
			       argv[0], TOKEN_PATH_NAME);
			goto exit;

		case 'p':
			path = strdup(optarg);
			break;

		case 't':
			trans = 1;
			break;

		default:
			printf("bad argument(s). \n");
			goto exit;
		}
	}

	if (path) {
		printf("probe posting list under path: %s\n", path);
	} else {
		printf("no path specified.\n");
		goto exit;
	}

	math_inex_probe(path, trans, stdout);
	free(path);
exit:
	return 0;
}
