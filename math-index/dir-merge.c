#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "math-index.h"

struct dir_merge_args {
	math_index_t     index;
	struct subpaths *subpaths; /* other subpaths */
	dir_merge_callbk fun;
	void            *args;
	char           (*base_paths)[MAX_DIR_PATH_NAME_LEN];
	char           (*full_paths)[MAX_DIR_PATH_NAME_LEN];
};

static void _show_paths(const char *base0, const char *dir0,
                        char (*paths)[MAX_DIR_PATH_NAME_LEN], uint32_t n)
{
	uint32_t i;
	printf("[0] %s/%s\n", base0, dir0);
	for (i = 1; i < n; i++)
		printf("[%u] %s\n", i, paths[i]);
}

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	uint32_t i;
	P_CAST(dm_args, struct dir_merge_args, arg);

	for (i = 1; i < dm_args->subpaths->n_lr_paths; i++) {
		sprintf(dm_args->full_paths[i], "%s/%s",
		        dm_args->base_paths[i], srchpath);

		if (!dir_exists(dm_args->full_paths[i])) {
			printf("stop subdir @ %s/%s.\n", path, srchpath);
			return DS_RET_STOP_SUBDIR;
		}
	}

	printf("================\n");
	_show_paths(path, srchpath, dm_args->full_paths,
	            dm_args->subpaths->n_lr_paths);

	return DS_RET_CONTINUE;
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

struct _dir_merge_init_args {
	math_index_t    index;
	struct subpath *one_subpath;
	char           (*paths)[MAX_DIR_PATH_NAME_LEN];
	uint32_t       idx;
};

static LIST_IT_CALLBK(dir_merge_init)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(args, struct _dir_merge_init_args, pa_extra);

	if (args->one_subpath == NULL)
		args->one_subpath = sp;

	math_index_mk_path_str(args->index, sp,
	                       args->paths[args->idx]);
	//printf("[%u]: %s\n", args->idx, args->paths[args->idx]);
	
	args->idx ++;
	LIST_GO_OVER;
}

static struct subpath *
init_dir_merge(math_index_t index, struct subpaths *subpaths,
               char (*paths)[MAX_DIR_PATH_NAME_LEN])
{
	struct _dir_merge_init_args init_args = {index, NULL, paths, 0};
	list_foreach(&subpaths->li, &dir_merge_init, &init_args);

	return init_args.one_subpath;
}

#define NEW_SUBPATHS_DIR_BUF \
	malloc(subpaths->n_lr_paths * MAX_DIR_PATH_NAME_LEN);

int math_index_dir_merge(math_index_t index, enum dir_merge_type type,
                         struct subpaths *subpaths, dir_merge_callbk fun,
                         void *args)
{
	int ret = 0;
	struct subpath *one_subpath;
	struct dir_merge_args dm_args = {index, subpaths, fun, args, NULL, NULL};

	dm_args.base_paths = NEW_SUBPATHS_DIR_BUF;
	dm_args.full_paths = NEW_SUBPATHS_DIR_BUF;

//	_debug_del_pathid(subpaths, 1);
//	_debug_del_pathid(subpaths, 2);
//	_debug_del_pathid(subpaths, 4);
//	printf("now:\n");
//	subpaths_print(subpaths, stdout);

	strip_gener_paths(subpaths);

//	printf("after:\n");
//	subpaths_print(subpaths, stdout);

	one_subpath = init_dir_merge(index, subpaths, dm_args.base_paths);

	if (one_subpath == NULL) {
		ret = 1;
		goto exit;
	}

	_debug_del_pathid(subpaths, one_subpath->path_id);

	if (type == DIR_MERGE_DEPTH_FIRST) {

		dir_search_podfs(dm_args.base_paths[0], &dir_search_callbk,
		                 &dm_args);
	} else {
		; /* not implemented */
	}

exit:
	free(dm_args.base_paths);
	free(dm_args.full_paths);

	return ret;
}
