#include <stdio.h>
#include <string.h>

#include "jieba-wrap.h"

int main(int argc, char *argv[])
{
	void *gen_toks; int i;
	const char *test_strings[] = {
	"其实，工信处女干事sandy每月经过下属办公室",
	"某公司正在研发石墨烯超导材料",
	};
	const int n_test_strings = sizeof test_strings / sizeof(char*);

	jieba_init();
	jieba_add_usr_word("石墨烯", strlen("石墨烯"));

	for (i = 0; i < n_test_strings; i++) {
		printf("\n");
		printf("#%d: %s\n", i, test_strings[i]);

		gen_toks = jieba_cut(test_strings[i],
		                     strlen(test_strings[i]));
		if (gen_toks) {
			foreach_tok(gen_toks, &jieba_token_print, NULL);

			//PRINT_REF_CNT(gen_toks);
			pyobj_refcnt(gen_toks);
		}
	}

	jieba_release();
	return 0;
}
