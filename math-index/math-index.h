#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "tex-parser/tex-parser.h" /* for subpaths */
#include "term-index/term-index.h" /* for doc_id_t */

#define MAX_MATH_PATHS 128
typedef uint8_t  pathinfo_num_t;

typedef void* math_index_t;
typedef uint32_t exp_id_t;

/* ======================
 * math index open/close
 * ====================== */
enum math_index_open_opt {
	MATH_INDEX_READ_ONLY,
	MATH_INDEX_WRITE
};

math_index_t
math_index_open(const char *, enum math_index_open_opt);

int math_index_add_tex(math_index_t, doc_id_t, exp_id_t, struct subpaths);

int math_inex_probe(const char*, bool, FILE*);

void math_index_close(math_index_t);

/* ==================
 * math posting list
 * ================== */
#pragma pack(push, 1)
struct math_posting_item {
	doc_id_t  doc_id;
	exp_id_t  exp_id;
	uint64_t  pathinfo_pos;
};

struct math_pathinfo {
	uint32_t    path_id;
	symbol_id_t lf_symb;
	symbol_id_t fr_hash;
};

struct math_pathinfo_pack {
	pathinfo_num_t        n_paths;
	uint32_t              n_lr_paths;
	struct math_pathinfo  pathinfo[0];
};
#pragma pack(pop)

/* =============================
 * get/free posting list handle
 * ============================= */
typedef void* math_posting_t;

/* map posting list: if not exists, create it */
math_posting_t
math_index_map_posting(math_index_t*, struct subpath);

void math_index_release_posting(math_posting_t);

/* =========================
 * math posting list reader
 * ========================= */
void math_posting_start(math_posting_t);
bool math_posting_jump(math_posting_t, uint64_t);
void math_posting_next(math_posting_t);
void math_posting_finish(math_posting_t);

struct math_posting_item*
math_posting_current(math_posting_t);

struct math_pathinfo_pack*
math_posting_get_pathinfo(math_posting_t, struct math_posting_item*);

/* ===========================
 * math index directory merge
 * =========================== */
#include "dir-merge.h"
