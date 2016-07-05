#include "cppjieba/Jieba.hpp"
#include "txt-seg.h"

using namespace std;

static const char DICT_PATH[] = "../cppjieba/fork/dict/jieba.dict.utf8";
static const char HMM_PATH[] =  "../cppjieba/fork/dict/hmm_model.utf8";
static const char USER_DICT_PATH[] = "../cppjieba/fork/dict/user.dict.utf8";

cppjieba::Jieba *jieba = NULL;

int text_segment_init(const char *dict_path)
{
	dict_path = NULL; /* using hard-coded paths for now */

	jieba = new cppjieba::Jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH);
	if (jieba == NULL)
		return 1;

	jieba->SetQuerySegmentThreshold(2);
	return 0;
}

void text_segment_free()
{
	if (jieba) {
		delete jieba;
		jieba = NULL;
	}
}

list text_segment(const char *text)
{
	list ret = LIST_NULL;
	vector<cppjieba::Word> output_tokens;
	struct text_seg *seg;

	jieba->CutForSearch(text, output_tokens);

	vector<cppjieba::Word>::iterator it;
	for (it = output_tokens.begin(); it != output_tokens.end(); it++) {
		string tag = jieba->LookupTag(it->word);
//		cout<<tag<<endl;
		if (tag == "x") /* skip punctuation */
			continue;
		
		seg = (struct text_seg*)malloc(sizeof(struct text_seg));

		strcpy(seg->str, it->word.c_str());
		seg->offset = it->offset;
		seg->n_bytes = strlen(seg->str);
		LIST_NODE_CONS(seg->ln);

		list_insert_one_at_tail(&seg->ln, &ret, NULL, NULL);

//		cout<<it->word<<endl;
//		cout<<it->offset<<endl;
	}

	return ret;
}
