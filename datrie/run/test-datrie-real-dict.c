#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mhook/mhook.h"
#include "datrie.h"

int main()
{
	char utf8_str[2048] = "彩票潮";
	unsigned int df, cnt = 0;
	datrie_state_t ret;
	struct datrie dict;

	FILE *dict_list = fopen("chinese-dict.txt", "r");
	if (NULL == dict_list) {
		printf("cannot open dict text file. \n");
		return 1;
	}

	dict = datrie_new();
	while (EOF != fscanf(dict_list, "%s %u", utf8_str, &df)) {
		printf("word: %s (df=%u), \n", utf8_str, df);
		ret = datrie_insert(&dict, utf8_str);
		printf("ret: %u, len: %u\n", ret, dict.len);

//		if ((++cnt) > 10000)
//			break;
	}
	datrie_free(dict);

	fclose(dict_list);
	mhook_print_unfree();
	return 0;
}

//	ret = datrie_lookup(&dict, test_string[i]);
