#include <stdio.h>
#include <stdlib.h>

#include "src/jieba.h"

void CutDemo() {
  printf("CutDemo:\n");
  static const char* DICT_PATH = "./dict/jieba.dict.utf8";
  static const char* HMM_PATH = "./dict/hmm_model.utf8";
  static const char* USER_DICT = "./dict/user.dict.utf8";

  // init will take a few seconds to load dicts.
  Jieba handle = NewJieba(DICT_PATH, HMM_PATH, USER_DICT);
  char input[300];
  printf("Please input some chinese: ");//add
  fgets (input, 300, stdin);
  //scanf ("%[^\n]%*c", input);

//"I love NYC. new york is a big city. hello world? 南京市长江大桥.有一只快速狐狸跳过了懒惰狗狗，又有一个孟尧吃了能量棒，地下室的电视机不停地播放着电视节目，但是我们不爱看，究竟爱上一个男人需要花上多少时间"

  char** words = Cut(handle, input);
  char** x = words;
  while (x && *x) {
    printf("%s\n", *x);
    x++;
  }
  FreeWords(words);
  FreeJieba(handle);
}

void ExtractDemo() {
  printf("ExtractDemo:\n");
  static const char* DICT_PATH = "./dict/jieba.dict.utf8";
  static const char* HMM_PATH = "./dict/hmm_model.utf8";
  static const char* IDF_PATH = "./dict/idf.utf8";
  static const char* STOP_WORDS_PATH = "./dict/stop_words.utf8";
  static const char* USER_DICT = "./dict/user.dict.utf8";

  // init will take a few seconds to load dicts.
  Extractor handle = NewExtractor(DICT_PATH,
        HMM_PATH,
        IDF_PATH,
        STOP_WORDS_PATH,
        USER_DICT);

  char** words = Extract(handle, "我mengyao是拖拉机学院手扶拖拉机专业的。不用多久，我就会升职加薪，当上CEO，走上人生巅峰。I like to do this", 5);
  char** x = words;
  while (x && *x) {
    printf("%s\n", *x);
    x++;
  }
  FreeWords(words);
  FreeExtractor(handle);
}

int main(int argc, char** argv) {
  CutDemo();
  //ExtractDemo();
  return 0;
}
