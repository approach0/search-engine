struct subpaths;

enum dir_merge_ret {
	DIR_MERGE_RET_CONTINUE,
	DIR_MERGE_RET_STOP
};

enum dir_merge_type {
	DIR_MERGE_DIRECT,
	DIR_MERGE_DEPTH_FIRST,
	DIR_MERGE_BREADTH_FIRST
};

enum dir_merge_pathset_type {
	DIR_PATHSET_PREFIX_PATH,
	DIR_PATHSET_LEAFROOT_PATH
};

#include "config.h" /* for MAX_MERGE_DIRS */
typedef enum dir_merge_ret
(*dir_merge_callbk)(
	char (*full_paths)[MAX_DIR_PATH_NAME_LEN],
	char (*base_paths)[MAX_DIR_PATH_NAME_LEN],
	struct subpath_ele          **eles,
	uint32_t /* number of postings list */,
	uint32_t /* level */, void *args);

list
dir_merge_subpath_set(enum dir_merge_pathset_type, struct subpaths*, int*);

#include "list/list.h"
int math_index_dir_merge(
	math_index_t,
	enum dir_merge_type,
	enum dir_merge_pathset_type,
	list subpath_set, int,
	dir_merge_callbk, void*
);
