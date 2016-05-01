#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jieba-wrap.h"
#include "wstring/wstring.h"
#include "txt-seg.h"

int text_segment_init(const char *dict_path)
{
	dict_path = NULL; /* not supported yet */
	jieba_init();
	return 0;
}

void text_segment_free()
{
	jieba_release();
}

bool text_segment_insert_usrterm(const char *term)
{
	jieba_add_usr_word(term, strlen(term));
	return 1;
}

void insert_token_to_list(char *term, long int begin,
                          long int end, char *tag, void *arg)
{
	struct term_list_node *tln;
	P_CAST(ret, list, arg);
	size_t len = end - begin;    /* characters */

	if (0 == strcmp(tag, "x")) /* drop unknown terms */
		return;

	if (0 == strcmp(tag, "m")) /* drop number terms */
		return;

	if (len + 1 > MAX_TERM_WSTR_LEN)
		return;
	
	tln = malloc(sizeof(struct term_list_node));
	wstr_copy(tln->term, mbstr2wstr(term));
	tln->begin_pos = begin;
	tln->end_pos = end;

	if (0 == strcmp(tag, "eng")) /* drop number terms */
		tln->type = TERM_T_ENGLISH;
	else
		tln->type = TERM_T_CHINESE;

	LIST_NODE_CONS(tln->ln);
	list_insert_one_at_tail(&tln->ln, ret, NULL, NULL);
}

list text_segment(const char *text)
{
	list ret = LIST_NULL;
	void *gen_toks = jieba_cut(text, strlen(text));
	if (gen_toks) {
		foreach_tok(gen_toks, &insert_token_to_list, &ret);
		pyobj_refcnt(gen_toks);
	}

	return ret;
}
