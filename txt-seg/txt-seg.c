#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

bool text_segment_insert_usrterm(const char *term)
{
	return JiebaInsertUserWord(jieba, term);
}

list text_segment(const char *text)
{
	CJiebaWord *words = CutNoPunc(jieba, text, strlen(text));
	int i;
	struct term_list_node *tln;
	list ret = LIST_NULL;
	const int max_word_bytes = 128;
	char word[max_word_bytes + 1];

	for (i = 0; words[i].word; i++) {
		if (words[i].len > max_word_bytes)
			continue;

		tln = malloc(sizeof(struct term_list_node));
		memcpy(word, words[i].word, words[i].len);
		word[words[i].len] = '\0';
		mbstr_copy(tln->term, mbstr2wstr(word));

		LIST_NODE_CONS(tln->ln);
		list_insert_one_at_tail(&tln->ln, &ret, NULL, NULL);
	}

	FreeWords(words);
	return ret;
}
