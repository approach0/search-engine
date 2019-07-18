#include "dir-util/dir-util.h" /* for MAX_DIR_PATH_NAME_LEN */
#include "term-index/term-index.h" /* for doc_id_t and position_t */
#include "tex-parser/tex-parser.h" /* for subpaths */

typedef position_t exp_id_t;
#define MAX_EXP_ID UINT_MAX

typedef struct math_index {
	char dir[MAX_DIR_PATH_NAME_LEN];
	char mode[8];
} *math_index_t;

math_index_t math_index_open(const char*, const char*);

void math_index_close(math_index_t);

int math_index_add(math_index_t, doc_id_t, exp_id_t, struct subpaths);

int mk_path_str(struct subpath*, int, char*);
