#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* dependency modules */
#include "list/list.h"
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "keyval-db/keyval-db.h"
#include "txt-seg/txt-seg.h"
#include "dir-util/dir-util.h"

/* lexer function type */
typedef void (*file_lexer)(const char*);

/* different lexer functions */
void lex_file_mix(const char*);
void lex_file_eng(const char*);

/* segment position to offset map */
#pragma pack(push, 1)

typedef struct {
	doc_id_t docID;
	position_t pos;
} offsetmap_from_t;

typedef struct {
	uint32_t offset, n_bytes;
} offsetmap_to_t;

#pragma pack(pop)

/* lex slice structure */
enum lex_slice_type {
	LEX_SLICE_TYPE_MATH,
	LEX_SLICE_TYPE_ENG_TEXT,
	LEX_SLICE_TYPE_TEXT
};

struct lex_slice {
	char  *mb_str;    /* in multi-bytes */
	uint32_t offset;   /* in bytes */
	enum lex_slice_type type;
};

/* main indexing functions */
void index_set(void *, math_index_t, keyval_db_t);
void index_file(const char*, file_lexer);
void indexer_handle_slice(struct lex_slice*);

/* offset checking test utilities */
#include "offset-check.h"

/* other utilities */
#include <ctype.h> /* for tolower() */
static __inline void eng_to_lower_case(char *str, size_t n_bytes)
{
	size_t i;
	for(i = 0; i < n_bytes; i++)
		str[i] = tolower(str[i]);
}

static __inline void strip_math_tag(char *str, size_t n_bytes)
{
	size_t tag_sz = strlen("[imath]");
	uint32_t i;
	for (i = 0; tag_sz + i + 1 < n_bytes - tag_sz; i++) {
		str[i] = str[tag_sz + i];
	}

	str[i] = '\0';
}
