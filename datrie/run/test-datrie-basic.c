#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mhook/mhook.h"
#include "datrie.h"

int main()
{
	datrie_state_t ret, i;
	struct datrie dict;
	const char test_string[][64] = {
		"C",
		"bachelor",
		"jar",
		"air",
		"badge",
		"baby",
		"bad",
		"badly",
		"badly",   /* test repeating insert */
		"boy",
		"北京",        /* utf-8 non-alphabet */
		"北京颐和园",  /* utf-8 non-alphabet */
		"apple",
		"app",
		"toolong", /* lookup failure test case */
		"ba"       /* lookup failure test case */
	};

	dict = datrie_new();
	datrie_print(dict, 0);

	for (i = 0; i < sizeof(test_string) / 64 - 2; i++) {
		ret = datrie_insert(&dict, test_string[i]);
		printf("\n");
		printf("inserting %s (return %u)\n", test_string[i], ret);
		for (int j = 0; j < strlen(test_string[i]); j++) {
			printf("\t `%c' (+%u)\n", test_string[i][j],
			                          datrie_cmap(test_string[i][j]));
		}
		datrie_print(dict, 0);
	}

	for (i = 0; i < sizeof(test_string) / 64; i++) {
		ret = datrie_lookup(&dict, test_string[i]);
		printf("\n");
		printf("finding: %s (return %u)\n", test_string[i], ret);
		for (int j = 0; j < strlen(test_string[i]); j++) {
			printf("\t `%c' (+%u)\n", test_string[i][j],
			                          datrie_cmap(test_string[i][j]));
		}
	}

	datrie_free(dict);
	mhook_print_unfree();
	return 0;
}
