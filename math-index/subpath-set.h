#include <stdint.h>

struct subpath_ele {
	list path_nodes;
	struct math_posting_item to_write;
	struct list_node ln;
	enum subpath_type subpath_type;
	uint32_t dup_cnt;
};

LIST_DECL_FREE_FUN(subpath_set_free);

void subpath_set_add(list*, struct subpath*, struct math_posting_item*);
void subpath_set_print(list*, FILE*);
