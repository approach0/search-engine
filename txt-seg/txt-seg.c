#include <stddef.h>
//#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "list.h"
#include "jieba/cjieba/src/jieba.h"

struct term_list_node {
	char          term[MAX_TERM_STR_LEN];
	struct list_node ln;
};
static const char* DICT_PATH = "jieba/cjieba/dict/jieba.dict.utf8";
static const char* HMM_PATH = "jieba/cjieba/dict/hmm_model.utf8";
static const char* USER_DICT = "jieba/cjieba/dict/user.dict.utf8";
Jieba handle;

void text_segment_init(){
	handle= NewJieba(DICT_PATH, HMM_PATH, USER_DICT);
}
void text_segment_FreeJieba(){
	FreeJieba(handle);
}

//void CutDemo()
list text_segment(char *text, int len);{
  printf("CutDemo:\n");
  // init will take a few seconds to load dicts.
  //char input[300];
  //printf("Please input some chinese: ");//add
  //fgets (input, 300, stdin);
  //scanf ("%[^\n]%*c", input);

//"I love NYC. new york is a big city. hello world? 南京市长江大桥.有一只快速狐狸跳过了懒惰狗狗，又有一个孟尧吃了能量棒，地下室的电视机不停地播放着电视节目，但是我们不爱看，究竟爱上一个男人需要花上多少时间"

  char** words = Cut(handle, text);
  char** x = words;
  while (x && *x) {
    //printf("%s\n", *x);
		new term_list_node{

		}
    x++;
  }
  FreeWords(words);
}






int main(int argc, char** argv) {
  CutDemo();
  //ExtractDemo();
  return 0;
}
