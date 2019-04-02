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
#include "wstring/wstring.h"

/* tex-parse counters */
extern uint64_t n_parse_err;
extern uint64_t n_parse_tex;

/* tex-parse exception handler */
typedef int (*indexer_tex_exception_handler)(char *, char *, uint64_t, uint64_t);
extern indexer_tex_exception_handler on_tex_index_exception;

/* main indexing functions */
doc_id_t indexer_assign(struct indices*);

typedef int (*text_lexer)(FILE*);
int indexer_index_json_file(FILE*, text_lexer);
int indexer_index_json(const char *, text_lexer);

int indexer_handle_slice(struct lex_slice*);

int index_maintain();
int index_should_maintain();
size_t index_size();
int index_write();

/* other utilities */
static __inline void strip_math_tag(char *str, size_t n_bytes)
{
	size_t tag_sz = strlen("[imath]");
	uint32_t i;
	for (i = 0; tag_sz + i + 1 < n_bytes - tag_sz; i++) {
		str[i] = str[tag_sz + i];
	}

	str[i] = '\0';
}

static __inline bool json_ext(const char *filename)
{
	char *ext = filename_ext(filename);
	return (ext && strcmp(ext, ".json") == 0);
}

uint64_t total_json_files(const char *dir);

doc_id_t index_get_prev_docID();
