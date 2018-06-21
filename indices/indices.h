/*
 * indices structures
 */
#include "list/list.h"
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "blob-index/blob-index.h"
#include "postlist-cache/postlist-cache.h"

enum indices_open_mode {
	INDICES_OPEN_RD,
	INDICES_OPEN_RW
};

struct indices {
	term_index_t  ti;
	math_index_t  mi;
	blob_index_t  url_bi;
	blob_index_t  txt_bi;
	struct postlist_cache ci;
};

void indices_init(struct indices*);
bool indices_open(struct indices*, const char*, enum indices_open_mode);
void indices_close(struct indices*);
int  indices_cache(struct indices*);
