#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "linkli/list.h"
#include "dir-util/dir-util.h"
#include "tex-parser/head.h"

#include "config.h"
#include "math-index.h"

struct math_index_args {
	int prefix_len;
	math_index_t index;
	struct subpaths subpaths;
	linkli_t *li;
	int n_added;
};

struct mk_path_str_arg {
	char **dest;
	bool skip_first;
	int max;
	int cnt;
};

math_index_t
math_index_open(const char *path, const char *mode)
{
	math_index_t index;

	index = malloc(sizeof(struct math_index));

	sprintf(index->dir, "%s", path);
	sprintf(index->mode, "%s", mode);

	if (mode[0] == 'w') {
		mkdir_p(path);
		return index;

	} else if (mode[0] == 'r') {
		if (dir_exists(path))
			return index;
	}

	free(index);
	return NULL;
}

void math_index_close(math_index_t index)
{
	free(index);
}

static int prefix_path_level(struct subpath *sp, int prefix_len)
{
	if (prefix_len > sp->n_nodes)
		return 1;

	return sp->n_nodes - prefix_len;
}

static LIST_IT_CALLBK(_mk_path_str)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct mk_path_str_arg, pa_extra);

	if (arg->skip_first) {
		arg->skip_first = 0;
		goto next;
	}

	if (arg->cnt == arg->max) {
		return LIST_RET_BREAK;
	}
	arg->cnt ++;

	*(arg->dest) += sprintf(*(arg->dest), "/%s", trans_token(sp_nd->token_id));

next:
	LIST_GO_OVER;
}

int mk_path_str(struct subpath *sp, int prefix_len, char *dest)
{
	struct mk_path_str_arg arg = {&dest, 0, prefix_len, 0};

	if (prefix_len > sp->n_nodes)
		return 2;

	if (sp->type == SUBPATH_TYPE_GENERNODE ||
	    sp->type  == SUBPATH_TYPE_WILDCARD) {
		dest += sprintf(dest, "%s", GENER_PATH_NAME);
		arg.skip_first = 1;
		arg.max -= 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_NORMAL) {
		dest += sprintf(dest, "%s", PREFIX_PATH_NAME);
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else {
		return 1;
	}

	return 0;
}

static LIST_IT_CALLBK(mkdir_and_setify)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(args, struct math_index_args, pa_extra);

	char path[MAX_DIR_PATH_NAME_LEN] = "";
	char *append = path;
	append += sprintf(append, "%s/", args->index->dir);

	/* sanity check */
	if (sp->type != SUBPATH_TYPE_NORMAL &&
	    sp->type != SUBPATH_TYPE_GENERNODE) {
		LIST_GO_OVER;
	}

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		int level = prefix_path_level(sp, args->prefix_len);

		/* since gener paths are plenty, only index high subtrees here */
		if (level > MAX_WILDCARD_LEVEL) {
			LIST_GO_OVER;
		}
	}

	if (0 != mk_path_str(sp, args->prefix_len, append)) {
		/* specified prefix_len is greater than actual path length */
		LIST_GO_OVER;
	}

	/* make directory */
	printf("%s\n", path);
	mkdir_p(path);

	/* add into subpath set */
//	(void)subpath_set_add(&arg->subpath_set, sp, arg->prefix_len, 0);

	LIST_GO_OVER;
}

int math_index_add(math_index_t index, doc_id_t docID, exp_id_t expID,
                   struct subpaths subpaths)
{
	if (subpaths.n_lr_paths > MAX_MATH_PATHS) {
		prerr("too many subpaths (%lu > %lu), abort.\n",
			subpaths.n_lr_paths, MAX_MATH_PATHS);
		return 1;
	}

	linkli_t subpath_set = NULL;
	struct math_index_args args = {0, index, subpaths, &subpath_set, 0};
	for (args.prefix_len = 2;; args.prefix_len ++) {
		list_foreach(&subpaths.li, &mkdir_and_setify, &args);

		if (subpath_set == NULL) break;

//		list_foreach(&arg.subpath_set, &set_write_to_index, &arg);
//		subpath_set_free(&arg.subpath_set);
	}

	return 0;
}
