#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "dir-util/dir-util.h" /* for MAX_DIR_PATH_NAME_LEN */
#include "tex-parser/tex-parser.h" /* for subpaths */
#include "term-index/term-index.h" /* for doc_id_t and position_t */

#define MAX_MATH_PATHS MAX_SUBPATH_ID
#define MAX_PREFIX_QUERY_MATH_PATHS (MAX_SUBPATH_ID * 4)

typedef uint8_t  pathinfo_num_t;

/* define math expression ID type */
typedef position_t exp_id_t;
#define MAX_EXP_ID UINT_MAX

/* ===================
 * math index general
 * =================== */
enum math_index_open_opt {
	MATH_INDEX_READ_ONLY,
	MATH_INDEX_WRITE
};

enum math_index_wr_opt {
	MATH_INDEX_WO_PREFIX,
	MATH_INDEX_ALL
};

typedef struct math_index {
	enum math_index_open_opt open_opt;
	char dir[MAX_DIR_PATH_NAME_LEN];
} *math_index_t;

math_index_t
math_index_open(const char *, enum math_index_open_opt);

int math_index_add_tex(math_index_t, doc_id_t, exp_id_t,
                       struct subpaths, enum math_index_wr_opt);

bool math_index_mk_path_str(struct subpath*, char*);
bool math_index_mk_prefix_path_str(struct subpath*, int, char*);

/* two debug functions */
int math_inex_probe_v1(const char*, bool, FILE*);
int math_inex_probe_v2(const char*, bool, FILE*);

void math_index_close(math_index_t);

/* ==================
 * math posting list
 * ================== */
#include "math-posting.h"

/* ===========================
 * math index directory merge
 * =========================== */
#include "dir-merge.h"

void delete_gener_paths(struct subpaths*);
