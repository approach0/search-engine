#include "dir-util/dir-util.h" /* for MAX_DIR_PATH_NAME_LEN */
#include "term-index/term-index.h" /* for doc_id_t and position_t */
#include "tex-parser/tex-parser.h" /* for subpaths */

typedef position_t exp_id_t;
#define MAX_EXP_ID UINT_MAX

typedef struct math_index {
	char dir[MAX_DIR_PATH_NAME_LEN];
} *math_index_t;

int math_index_add(math_index_t, doc_id_t, exp_id_t, struct subpaths);
