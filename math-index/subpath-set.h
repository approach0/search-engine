#include <stdint.h>

struct subpath_ele {
	struct list_node ln;
	uint32_t         dup_cnt;
	struct subpath  *dup[MAX_MATH_PATHS];
};

LIST_DECL_FREE_FUN(subpath_set_free);

void subpath_set_add(list*, struct subpath*);
void subpath_set_print(list*, FILE*);
