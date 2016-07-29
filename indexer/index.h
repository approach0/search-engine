#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* dependency modules */
#include "txt-seg/config.h"
#include "txt-seg/txt-seg.h"
#include "txt-seg/lex.h"
#include "dir-util/dir-util.h"
#include "codec/codec.h"
#include "indices/indices.h"

/* main indexing functions */
void indexer_assign(struct indices*);

typedef void (*text_lexer)(FILE*);
void indexer_index_json(FILE*, text_lexer);

void indexer_handle_slice(struct lex_slice*);

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
