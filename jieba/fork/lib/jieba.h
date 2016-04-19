#ifndef CJIEBA_C_API_H
#define CJIEBA_C_API_H

#include <stdlib.h>
#include <stdbool.h>

typedef void* Jieba;
Jieba NewJieba(const char* dict_path, const char* hmm_path, const char* user_dict);
void FreeJieba(Jieba);

typedef struct {
  const char* word;
  size_t len;
} CJiebaWord;

CJiebaWord* Cut(Jieba handle, const char* sentence, size_t len);

CJiebaWord* CutNoPunc(Jieba handle, const char* sentence, size_t len);

void FreeWords(CJiebaWord* words);

bool JiebaInsertUserWord(Jieba handle, const char* word);

typedef void* Extractor;
Extractor NewExtractor(const char* dict_path,
      const char* hmm_path,
      const char* idf_path,
      const char* stop_word_path,
      const char* user_dict_path);
CJiebaWord* Extract(Extractor handle, const char* sentence, size_t len, size_t topn);
void FreeExtractor(Extractor handle);

#endif
