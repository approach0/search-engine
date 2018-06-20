/*
 * indices structures
 */
#include "list/list.h"
#include "term-index/term-index.h"
#include "math-index/math-index.h"
#include "blob-index/blob-index.h"

enum indices_open_mode {
	INDICES_OPEN_RD,
	INDICES_OPEN_RW
};

struct indices {
	void                 *ti;
	math_index_t          mi;
	blob_index_t          url_bi;
	blob_index_t          txt_bi;
};

void indices_init(struct indices*);
bool indices_open(struct indices*, const char*, enum indices_open_mode);
void indices_close(struct indices*);
