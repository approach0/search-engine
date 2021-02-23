#include "linkli/list.h"
#include "math-index-v3/math-index.h"
#include "math-index-v3/subpath-set.h"
#include "merger/mergers.h"
#include "mnc-score.h"

struct math_qry {
	const char *tex;
	void       *optr;
	int         n_qnodes;

	struct subpaths lrpaths;

	linkli_t    subpath_set;
	merge_set_t merge_set;

	struct math_invlist_entry_reader entry[MAX_MERGE_SET_SZ];

	/* shortcut links to subpath set elements */
	struct subpath_ele *ele[MAX_MERGE_SET_SZ];

	struct mnc_scorer mnc; /* symbol scoring */
};

int  math_qry_prepare(math_index_t, const char*, struct math_qry*);
void math_qry_release(struct math_qry*);
void math_qry_print(struct math_qry*, int, FILE*);
