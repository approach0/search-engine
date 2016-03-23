#include <stdlib.h>
#include <stdio.h>
#include "jieba.h"
#include "wstring/wstring.h"
#include "txt-seg.h"

static Jieba jieba;

int text_segment_init(const char *dict_path)
{
	char paths[3][MAX_DICT_PATH_STR_LEN];

	sprintf(paths[0], "%s/" DEFAULT_DICT_NAME, dict_path);
	sprintf(paths[1], "%s/" DEFAULT_HMM_NAME, dict_path);
	sprintf(paths[2], "%s/" DEFAULT_USER_DICT_NAME, dict_path);

	jieba = NewJieba(paths[0], paths[1], paths[2]); 

	return 0;
}

void text_segment_free()
{
	FreeJieba(jieba);
}

list text_segment(const char *text)
{
	char **words = Cut(jieba, text);
	int i;
	struct term_list_node *tln;
	list ret = LIST_NULL;

	for (i = 0; words[i]; i++) {
		tln = malloc(sizeof(struct term_list_node));
		mbstr_copy(tln->term, mbstr2wstr(words[i]));

		LIST_NODE_CONS(tln->ln);
		list_insert_one_at_tail(&tln->ln, &ret, NULL, NULL);
	}

	FreeWords(words);
	return ret;
}
