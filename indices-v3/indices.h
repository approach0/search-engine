#pragma once
#include "config.h"

/*
 * indices structures
 */
#include "term-index/term-index.h"
#include "math-index-v3/math-index.h"
#include "blob-index/blob-index.h"

/*
 * Indices of mixed math tex and text terms
 */
enum indices_open_mode {
	INDICES_OPEN_RD,
	INDICES_OPEN_RW
};

struct indices {
	term_index_t  ti;
	math_index_t  mi;

	blob_index_t  url_bi;
	blob_index_t  txt_bi;

	/* specified cache memory limits */
	size_t mi_cache_limit;
	size_t ti_cache_limit;

	size_t memo_usage; /* of cache */

	/* index stats */
	uint32_t n_doc;
	uint32_t avgDocLen;
	uint32_t n_tex;
	uint32_t n_secttr;
};

int  indices_open(struct indices*, const char*, enum indices_open_mode);
void indices_close(struct indices*);

int  indices_cache(struct indices*);
void indices_print_summary(struct indices*);

/*
 * Indexer (writer)
 */
struct indexer;
typedef int (*parser_exception_callbk)(struct indexer *, char *tex, char *msg);

typedef int (*text_lexer)(FILE*);

struct indexer {
	struct indices *indices;

	/* fields */
	char url_field[MAX_CORPUS_FILE_SZ];
	char txt_field[MAX_CORPUS_FILE_SZ];

	uint32_t cur_position; /* current lexer position */

	/* parser error rate stats */
	uint64_t n_parse_err;
	uint64_t n_parse_tex;

	/* callback functions */
	text_lexer lexer;
	parser_exception_callbk on_parser_exception;
};

struct indexer *indexer_alloc(struct indices*, text_lexer,
                              parser_exception_callbk);
void            indexer_free(struct indexer*);

int             indexer_write_all_fields(struct indexer*);
