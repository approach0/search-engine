#pragma once
#include <stdint.h>

/* dependency modules */
#include "list/list.h"
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "keyval-db/keyval-db.h"
#include "txt-seg/txt-seg.h"
#include "dir-util/dir-util.h"

/* invoke lexer to parse a file */
void lex_file(const char*);

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
	LEX_SLICE_TYPE_TEXT
};

struct lex_slice {
	char  *mb_str;    /* in multi-bytes */
	uint32_t offset;   /* in bytes */
	enum lex_slice_type type;
};

/* offset checking test utilities */
#include "offset-check.h"

/* other utilities */
#include <ctype.h> /* for tolower() */
static __inline void eng_to_lower_case(struct lex_slice *slice)
{
	size_t i;
	for(i = 0; i < slice->offset; i++)
		slice->mb_str[i] = tolower(slice->mb_str[i]);
}

static __inline void strip_math_tag(char *text, size_t bytes)
{
	size_t pad = strlen("[imath]");
	uint32_t i;
	for (i = 0; pad + i + 1 < bytes - pad; i++) {
		text[i] = text[pad + i];
	}

	text[i] = '\0';
}
