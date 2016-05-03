#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "math-index.h"
#include "subpath-set.h"

struct dir_merge_args {
	math_index_t          index;
	dir_merge_callbk      fun;
	void                 *args;

	/* subpath set */
	uint32_t              set_sz;
	struct subpath_ele  **eles;

	/* dir-merge path strings */
	uint32_t       longpath;
	char           (*base_paths)[MAX_DIR_PATH_NAME_LEN];
	char           (*full_paths)[MAX_DIR_PATH_NAME_LEN];
};

/*
 * functions below are for debug purpose.
 */
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

static void del_subpath_of_path_id(struct subpaths *subpaths, uint32_t path_id)
{
	list_foreach(&subpaths->li, &dele_pathid, &path_id);
}

static void
print_all_dir_strings(struct dir_merge_args *dm_args, const char *srchpath)
{
	uint32_t i;

	printf("======\n");
	sprintf(dm_args->full_paths[dm_args->longpath], "%s/%s",
	        dm_args->base_paths[dm_args->longpath], srchpath);

	for (i = 0; i < dm_args->set_sz; i++)
		printf("[%u] %s\n", i, dm_args->full_paths[i]);
}

/*
 * directory search callback function.
 */
static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	uint32_t i;
	P_CAST(dm_args, struct dir_merge_args, arg);

	for (i = 0; i < dm_args->set_sz; i++) {
		if (i == dm_args->longpath)
			continue; /* skip caller path (longpath) */

		sprintf(dm_args->full_paths[i], "%s/%s",
		        dm_args->base_paths[i], srchpath);

		if (!dir_exists(dm_args->full_paths[i])) {
			printf("stop subdir merging @ %s/%s.\n", path, srchpath);
			return DS_RET_STOP_SUBDIR;
		}
	}

	print_all_dir_strings(dm_args, srchpath);
	return DS_RET_CONTINUE;
}

/*
 * dir-merge initialization related functions.
 */
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

struct mk_subpath_strings_args {
	uint32_t        i;
	math_index_t    index;
	uint32_t        longpath;
	size_t          max_strlen;
	char          (*paths)[MAX_DIR_PATH_NAME_LEN];
};

static LIST_IT_CALLBK(mk_subpath_strings)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(args, struct mk_subpath_strings_args, pa_extra);
	size_t len;

	/* make path string */
	math_index_mk_path_str(args->index, ele->dup[0],
	                       args->paths[args->i]);

	/* compare and update max string length */
	len = strlen(args->paths[args->i]);
	if (len > args->max_strlen) {
		args->max_strlen = len;
		args->longpath = args->i;
	}

	args->i ++;
	LIST_GO_OVER;
}

/*
 * math_index_dir_merge() main function.
 */
#define NEW_SUBPATHS_DIR_BUF(_n_paths) \
	malloc(_n_paths * MAX_DIR_PATH_NAME_LEN);

int math_index_dir_merge(math_index_t index, enum dir_merge_type type,
                         struct subpaths *subpaths, dir_merge_callbk fun,
                         void *args)
{
	int  i, n_uniq_paths, ret = 0;
	list subpath_set = LIST_NULL;
	struct mk_subpath_strings_args mkstr_args;

	struct dir_merge_args dm_args = {index, fun, args, 0 /* set size */,
	                                 NULL /* set elements */,
	                                 0 /* longpath index */,
	                                 NULL /* base paths */,
	                                 NULL /* full paths */};

	/* strip gener paths because they are not used for searching */
	list_foreach(&subpaths->li, &dele_if_gener, NULL);

	/* generate subpath set */
	n_uniq_paths = subpath_set_from_subpaths(subpaths, &subpath_set);

	/* allocate unique subpath string buffers */
	dm_args.set_sz = n_uniq_paths;
	dm_args.eles = malloc(sizeof(struct subpath_ele*) * n_uniq_paths);
	dm_args.longpath = 0 /* assign real number later */;
	dm_args.base_paths = NEW_SUBPATHS_DIR_BUF(n_uniq_paths);
	dm_args.full_paths = NEW_SUBPATHS_DIR_BUF(n_uniq_paths);

	/* make unique subpath strings and return longest path index (longpath) */
	mkstr_args.i = 0;
	mkstr_args.index = index;
	mkstr_args.longpath = 0;
	mkstr_args.max_strlen = 0;
	mkstr_args.paths = dm_args.base_paths;
	list_foreach(&subpath_set, &mk_subpath_strings, &mkstr_args);

	if (mkstr_args.i == 0) {
		fprintf(stderr, "no unique path for dir-merge.\n");
		ret = 1; goto exit;
	}

	printf("base path strings:\n");
	for (i = 0; i < n_uniq_paths; i++)
		printf("path string [%u]: %s\n", i, dm_args.base_paths[i]);

	/* assign real longpath index */
	dm_args.longpath = mkstr_args.longpath;
	printf("longest path : [%u] %s (length=%lu)\n", dm_args.longpath,
	       dm_args.base_paths[dm_args.longpath], mkstr_args.max_strlen);
	printf("\n");

	printf("subpath set (size=%u):\n", dm_args.set_sz);
	subpath_set_print(&subpath_set, stdout);
	printf("\n");

	/* now we can start merge path directories */
	if (type == DIR_MERGE_DEPTH_FIRST) {
		printf("start merge directories...\n");
		dir_search_podfs(dm_args.base_paths[dm_args.longpath],
		                 &dir_search_callbk, &dm_args);
	} else {
		; /* not implemented yet */
	}

exit:
	free(dm_args.eles);
	free(dm_args.base_paths);
	free(dm_args.full_paths);

	subpath_set_free(&subpath_set);
	return ret;
}
