#include <stdint.h>
#include <stdbool.h>
#include "list/list.h"

struct subpath_ele {
	struct list_node ln;
	uint32_t         dup_cnt;
	struct subpath  *dup[MAX_MATH_PATHS];
};

LIST_DECL_FREE_FUN(subpath_set_free);

typedef int subpath_set_comparer(struct subpath*, struct subpath*, int);

subpath_set_comparer sp_tokens_comparer;
subpath_set_comparer sp_prefix_comparer;

bool subpath_set_add(list*, struct subpath*, int, subpath_set_comparer*);
void subpath_set_print(list*, FILE*);

uint32_t /* return the number of unique subpaths added */
subpath_set_from_subpaths(struct subpaths*, subpath_set_comparer*,
                          list* /* set output */);

void delete_gener_paths(struct subpaths*);
