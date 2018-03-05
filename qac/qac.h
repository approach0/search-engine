#include <stdint.h>

#include "search/math-expr-search.h"
#include "search/config.h"
#include "search/search.h"
#include "search/search-utils.h"

typedef void qac_index_t;

enum qac_index_open_opt {
	QAC_INDEX_READ_ONLY,
	QAC_INDEX_WRITE_READ
};

struct qac_tex_info {
	uint32_t freq;
	uint32_t lr_paths;
};

void *qac_index_open(char*, enum qac_index_open_opt);
void  qac_index_close(qac_index_t *);

uint32_t math_qac_index_uniq_tex(qac_index_t*, const char*);

struct qac_tex_info math_qac_get(qac_index_t*, uint32_t, char **);

uint32_t qac_index_touch(void*, uint32_t);

ranked_results_t math_qac_query(qac_index_t*, const char*);

#define DEFAULT_QAC_SUGGESTIONS 10
