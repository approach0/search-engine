#include <stddef.h> /* for size_t */

void jieba_init(void);
void jieba_release(void);

void jieba_add_usr_word(const char*, size_t);

typedef void (*jieba_token_callbk)(char*, long int, long int, char*, void*);

void jieba_token_print(char*, long int, long int, char*, void*);

void *jieba_cut(const char*, size_t);
void foreach_tok(void*, jieba_token_callbk, void*);

long int pyobj_refcnt(void*);

#define PRINT_REF_CNT(_obj) \
	printf(#_obj " reference count: %ld @L%d\n", \
	       pyobj_refcnt(_obj), __LINE__)
