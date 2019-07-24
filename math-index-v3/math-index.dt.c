#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "linkli/list.h"
#include "dir-util/dir-util.h"
#include "tex-parser/head.h"

#include "config.h"
#include "math-index.h"
#include "subpath-set.h"

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

static void remove_wildcards(linkli_t *set)
{
	foreach (iter, li, *set) {
		struct subpath_ele *ele = li_entry(ele, iter->cur, ln);
		if (ele->dup[0]->type == SUBPATH_TYPE_WILDCARD) {
			iter->li = li_remove(set, iter->cur);
			free(ele);
		}
	}
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

static void make_dirs(linkli_t set, const char *root_name)
{
	foreach (iter, li, set) {
		char path[MAX_DIR_PATH_NAME_LEN] = "";
		char *append = path;
		append += sprintf(append, "%s/", root_name);

		struct subpath_ele *ele = li_entry(ele, iter->cur, ln);
		struct subpath *sp = ele->dup[0];

		if (0 != mk_path_str(sp, ele->prefix_len, append)) {
			/* specified prefix_len is greater than actual path length */
			continue;
		}

		/* make directory */
		printf("mkdir -p %s\n", path);
		mkdir_p(path);
	}
}

int math_index_add(math_index_t index, doc_id_t docID, exp_id_t expID,
                   struct subpaths subpaths)
{
	if (subpaths.n_lr_paths > MAX_MATH_PATHS) {
		prerr("too many subpaths (%lu > %lu), abort.\n",
			subpaths.n_lr_paths, MAX_MATH_PATHS);
		return 1;
	}

	linkli_t set = subpath_set(subpaths, SUBPATH_SET_DOC);	

	/* for indexing, we do not accept wildcards */
	remove_wildcards(&set);

	/* guarantee the corresponding directory is created */
	make_dirs(set, index->dir);

#ifdef DEBUG_SUBPATH_SET
	printf("subpath set (size=%d)\n", li_size(set));
	print_subpath_set(set);
#endif

	li_free(set, struct subpath_ele, ln, free(e));
	return 0;
}
