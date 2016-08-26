#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "head.h"

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

static void
del_subpath_of_path_id(struct subpaths *subpaths, uint32_t path_id)
{
	list_foreach(&subpaths->li, &dele_pathid, &path_id);
}

static void
print_all_dir_strings(struct dir_merge_args *dm_args)
{
	uint32_t i, j;
	struct subpath_ele *ele;

	for (i = 0; i < dm_args->set_sz; i++) {
		ele = dm_args->eles[i];
		printf("[%u] %s ", i, dm_args->full_paths[i]);

		printf("(duplicates: ");
		for (j = 0; j <= ele->dup_cnt; j++)
			printf("path#%u ", ele->dup[j]->path_id);
		printf(")\n");
	}
}

/*
 * directory search callback function.
 */
static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *arg)
{
	uint32_t i, j;
	enum ds_ret ret = DS_RET_CONTINUE;
	P_CAST(dm_args, struct dir_merge_args, arg);
	math_posting_t postings[MAX_MATH_PATHS];

	for (i = 0; i < dm_args->set_sz; i++) {
		sprintf(dm_args->full_paths[i], "%s/%s",
		        dm_args->base_paths[i], srchpath);

		postings[i] = math_posting_new_reader(dm_args->eles[i],
		                               dm_args->full_paths[i]);

		if (!dir_exists(dm_args->full_paths[i])) {
#ifdef DEBUG_DIR_MERGE
			printf("stop subdir merging @ %s/%s.\n", path, srchpath);
#endif
			ret = DS_RET_STOP_SUBDIR;
			i ++;
			goto free_postings;
		}
	}

#ifdef DEBUG_DIR_MERGE
	printf("post merging at directories:\n");
	print_all_dir_strings(dm_args);
	printf("\n");
#endif
	if (DIR_MERGE_RET_STOP == dm_args->fun(postings, dm_args->set_sz,
	                                       level, dm_args->args))
		ret = DS_RET_STOP_ALLDIR;

free_postings:
	for (j = 0; j < i; j++)
		math_posting_free_reader(postings[j]);

#ifdef DEBUG_DIR_MERGE
	printf("ret = %d.\n", ret);
#endif
	return ret;
}

/*
 * dir-merge initialization related functions.
 */
struct assoc_ele_and_pathstr_args {
	uint32_t             i;
	math_index_t         index;
	uint32_t             longpath;
	size_t               max_strlen;
	char               (*paths)[MAX_DIR_PATH_NAME_LEN];
	struct subpath_ele **eles;
};

static LIST_IT_CALLBK(assoc_ele_and_pathstr)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(args, struct assoc_ele_and_pathstr_args, pa_extra);
	char *append = args->paths[args->i];
	size_t len;

	/* make path string */
	append += sprintf(append, "%s/", args->index->dir);
	if (math_index_mk_path_str(ele->dup[0], append)) {
		args->i = 0; /* indicates error */
		return LIST_RET_BREAK;
	}

	/* associate paths[i] with a subpath set element */
	args->eles[args->i] = ele;

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
#ifdef DEBUG_DIR_MERGE
	int i;
#endif

	int  n_uniq_paths, ret = 0;
	list subpath_set = LIST_NULL;
	struct assoc_ele_and_pathstr_args assoc_args;

	struct dir_merge_args dm_args = {index, fun, args, 0 /* set size */,
	                                 NULL /* set elements */,
	                                 0 /* longpath index */,
	                                 NULL /* base paths */,
	                                 NULL /* full paths */};
	/* generate subpath set */
	n_uniq_paths = subpath_set_from_subpaths(subpaths, &subpath_set);

	/* allocate unique subpath string buffers */
	dm_args.set_sz = n_uniq_paths;
	dm_args.eles = malloc(sizeof(struct subpath_ele*) * n_uniq_paths);
	dm_args.longpath = 0 /* assign real number later */;
	dm_args.base_paths = NEW_SUBPATHS_DIR_BUF(n_uniq_paths);
	dm_args.full_paths = NEW_SUBPATHS_DIR_BUF(n_uniq_paths);

	/* make unique subpath strings and return longest path index (longpath) */
	assoc_args.i = 0;
	assoc_args.index = index;
	assoc_args.longpath = 0;
	assoc_args.max_strlen = 0;
	assoc_args.paths = dm_args.base_paths;
	assoc_args.eles = dm_args.eles;

	list_foreach(&subpath_set, &assoc_ele_and_pathstr, &assoc_args);

	if (assoc_args.i == 0) {
		/* last function fails for some reason */
		fprintf(stderr, "path strings are not fully generated.\n");
		ret = 1; goto exit;
	}

#ifdef DEBUG_DIR_MERGE
	printf("base path strings:\n");
	for (i = 0; i < n_uniq_paths; i++)
		printf("path string [%u]: %s\n", i, dm_args.base_paths[i]);
#endif

	/* now we get longpath index, we can pass it to dir-merge */
	dm_args.longpath = assoc_args.longpath;

#ifdef DEBUG_DIR_MERGE
	printf("longest path : [%u] %s (length=%lu)\n", dm_args.longpath,
	       dm_args.base_paths[dm_args.longpath], assoc_args.max_strlen);
	printf("\n");

	printf("subpath set (size=%u):\n", dm_args.set_sz);
	subpath_set_print(&subpath_set, stdout);
	printf("\n");
#endif

	/* now we can start merge path directories */
	if (type == DIR_MERGE_DEPTH_FIRST) {
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
