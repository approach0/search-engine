#ifndef CJIEBA_C_API_H
#define CJIEBA_C_API_H

typedef void* Jieba;
Jieba NewJieba(const char* dict_path, const char* hmm_path, const char* user_dict);
void FreeJieba(Jieba);
char** Cut(Jieba handle, const char* sentence);

void FreeWords(char** words);

typedef void* Extractor;
Extractor NewExtractor(const char* dict_path,
      const char* hmm_path,
      const char* idf_path,
      const char* stop_word_path,
      const char* user_dict_path);
char** Extract(Extractor handle, const char* sentence, int topn);
void FreeExtractor(Extractor handle);

#endif
