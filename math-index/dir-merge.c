#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "math-index.h"

struct dir_merge_args {
	math_index_t     index;
	struct subpaths *subpaths; /* other subpaths */
	dir_merge_callbk fun;
	void            *args;
};

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	P_CAST(dm_args, struct dir_merge_args, arg);
	printf("path=%s\n", path);
	printf("search path=%s\n", srchpath);

//	printf("files in this directory:\n");
//	foreach_files_in(path, &foreach_file_callbk, (void*)path);

	if (level > 3) {
		return DS_RET_STOP_ALLDIR;
	} else {
		return DS_RET_CONTINUE;
	}
}

static LIST_IT_CALLBK(dele_if_gener)
{
	bool res;
	LIST_OBJ(struct subpath, sp, ln);

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		res = list_detach_one(pa_now->now, pa_head, pa_now, pa_fwd);
		free(sp);
		return res;
	}

	LIST_GO_OVER;
}

static void strip_gener_paths(struct subpaths *subpaths)
{
	list_foreach(&subpaths->li, &dele_if_gener, NULL);
}

static LIST_IT_CALLBK(dele_pathid)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(path_id, uint32_t, pa_extra);

	if (sp->path_id == *path_id) {
		list_detach_one(pa_now->now, pa_head, pa_now, pa_fwd);
		free(sp);
		return LIST_RET_BREAK;
	}

	LIST_GO_OVER;
}

static void _debug_del_pathid(struct subpaths *subpaths, uint32_t path_id)
{
	list_foreach(&subpaths->li, &dele_pathid, &path_id);
}

int math_index_dir_merge(math_index_t index, enum dir_merge_type type,
                         struct subpaths *subpaths, dir_merge_callbk fun,
                         void *args)
{
	char path[MAX_DIR_PATH_NAME_LEN];
	char (*paths)[MAX_MATH_PATHS];
	struct subpath *one_subpath;
	struct dir_merge_args dm_args = {index, subpaths, fun, args};

//	_debug_del_pathid(subpaths, 1);
//	_debug_del_pathid(subpaths, 2);
//	_debug_del_pathid(subpaths, 4);
//	printf("now:\n");
//	subpaths_print(subpaths, stdout);

	strip_gener_paths(subpaths);

//	printf("after:\n");
	subpaths_print(subpaths, stdout);

	one_subpath = MEMBER_2_STRUCT(subpaths->li.now, struct subpath, ln);
	if (one_subpath == NULL)
		return 1;

	math_index_mk_path_str(index, one_subpath, path);
	_debug_del_pathid(subpaths, one_subpath->path_id);

	if (type == DIR_MERGE_DEPTH_FIRST) {
		dir_search_podfs(path, &dir_search_callbk, &dm_args);
	} else {
		; /* not implemented */
	}

	return 0;
}
