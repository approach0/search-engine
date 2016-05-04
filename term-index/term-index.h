#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t term_id_t;
typedef uint32_t doc_id_t;

enum term_index_open_flag {
	TERM_INDEX_OPEN_CREATE,
	TERM_INDEX_OPEN_EXISTS
};

void *term_index_open(const char *, enum term_index_open_flag);
void term_index_close(void *);

int term_index_maintain(void*);

void     term_index_doc_begin(void *);
void     term_index_doc_add(void *, char *);
doc_id_t term_index_doc_end(void *);

uint32_t term_index_get_termN(void *); /* unique terms */
/* Indri bug: call term_index_get_docN() during indexing will crash shortly */
uint32_t term_index_get_docN(void *); /* number of document in the collection */
uint32_t term_index_get_docLen(void *, doc_id_t); /* get document length */
uint32_t term_index_get_avgDocLen(void *); /* average doc len (in words) */
uint32_t term_index_get_df(void *, term_id_t); /* get document frequency */

term_id_t term_lookup(void *, char *);
char *term_lookup_r(void *, term_id_t);

void *term_index_get_posting(void *, term_id_t);

struct term_posting_item {
	doc_id_t doc_id;
	uint32_t tf;
};

bool term_posting_start(void *);
bool term_posting_jump(void *, uint64_t);
bool term_posting_next(void *);
struct term_posting_item
*term_posting_current(void *);
void term_posting_finish(void *);

#ifdef __cplusplus
}
#endif
