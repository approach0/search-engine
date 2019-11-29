#include "dir-util/dir-util.h"
#include "cppjieba/Jieba.hpp"

#include "config.h"
#include "txt-seg.h"

using namespace std;

#define DICT_NAME     "jieba.dict.utf8"
#define HMM_NAME      "hmm_model.utf8"
#define USR_DICT_NAME "user.dict.utf8"

cppjieba::Jieba *jieba = NULL;

int text_segment_init(const char *path)
{
	char dict_path[MAX_DIR_PATH_NAME_LEN];
	char hmm_path[MAX_DIR_PATH_NAME_LEN];
	char usr_dict_path[MAX_DIR_PATH_NAME_LEN];

	sprintf(dict_path, "%s/%s", path, DICT_NAME);
	sprintf(hmm_path, "%s/%s", path, HMM_NAME);
	sprintf(usr_dict_path, "%s/%s", path, USR_DICT_NAME);

	if (!file_exists(dict_path) ||
	    !file_exists(hmm_path) ||
	    !file_exists(usr_dict_path)) {
		fprintf(stderr, "Cannot open Jieba dict at (%s, %s, %s)\n",
		        dict_path, hmm_path, usr_dict_path);
		return 1;
	}

	jieba = new cppjieba::Jieba(dict_path, hmm_path,
	                            usr_dict_path);
	if (jieba == NULL)
		return 1;

	return 0;
}

void text_segment_free()
{
	if (jieba) {
		delete jieba;
		jieba = NULL;
	}
}

int text_segment_ready()
{
	return (jieba != NULL);
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

		strncpy(seg->str, it->word.c_str(), MAX_TXT_SEG_BYTES);
		seg->str[MAX_TXT_SEG_BYTES - 1] = '\0';

		seg->offset = it->offset;
		LIST_NODE_CONS(seg->ln);

		list_insert_one_at_tail(&seg->ln, &ret, NULL, NULL);

//		cout<<it->word<<endl;
//		cout<<it->offset<<endl;
	}

	return ret;
}
