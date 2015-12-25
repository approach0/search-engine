#include <stdlib.h>
#include <stdio.h>
#include "jieba.h"
#include "txt-seg.h"
#include "wstring.h"

static char DICT_PATH[] = "../cjieba/dict/jieba.dict.utf8";
static char HMM_PATH[] = "../cjieba/dict/hmm_model.utf8";
static char USER_DICT[] = "../cjieba/dict/user.dict.utf8";

static Jieba jieba;

int text_segment_init()
{
	jieba = NewJieba(DICT_PATH, HMM_PATH, USER_DICT); 
	return 0;
}

void text_segment_free()
{
	FreeJieba(jieba);
}

list text_segment(char *text)
{
	char **words = Cut(jieba, text);
	int i;
	struct term_list_node *tln;
	list ret;

	for (i = 0; words[i]; i++) {
		tln = malloc(sizeof(struct term_list_node));
		mbstr_copy(tln->term, mbstr2wstr(words[i]));

		LIST_NODE_CONS(tln->ln);
		list_insert_one_at_tail(&tln->ln, &ret, NULL, NULL);
	}

	FreeWords(words);
	return ret;
}
