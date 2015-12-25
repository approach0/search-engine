#include <stdio.h>
#include <stdlib.h>
#include "jieba.h"

#define JIEBA(_text) \
	words = Cut(jieba, _text); \
	for (i = 0; words[i]; i++) \
	printf("%s\n", words[i]); \
	FreeWords(words)

int main()
{
	const char* DICT_PATH = "../cjieba/dict/jieba.dict.utf8";
	const char* HMM_PATH = "../cjieba/dict/hmm_model.utf8";
	const char* USER_DICT = "../cjieba/dict/user.dict.utf8";
		
	Jieba jieba = NewJieba(DICT_PATH, HMM_PATH, USER_DICT); 
	int i;
	char** words;
	
	JIEBA("南京市长江大桥");
	JIEBA("C++和c#是什么关系？11+122=133，是吗？PI=3.14159");
	JIEBA("工信处女干事每月经过下属科室都要亲口交代24口交换机等技术性器件的安装工作");
	JIEBA("“Microsoft”一词由“MICROcomputer（微型计算机）”和“SOFTware（软件）”两部分组成");

	FreeJieba(jieba);
	return 0;
}
