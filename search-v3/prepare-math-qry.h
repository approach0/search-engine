#include "linkli/list.h"
#include "math-index-v3/math-index.h"
#include "math-index-v3/subpath-set.h"
#include "merger/mergers.h"

struct math_qry {
	const char *tex;
	void       *optr;

	struct subpaths subpaths;

	linkli_t    subpath_set;
	merge_set_t merge_set;
};

int  math_qry_prepare(math_index_t, const char*, struct math_qry*);
void math_qry_release(struct math_qry*);
