#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "common/common.h"
#include "list/list.h"
#include "tex-parser/tex-parser.h"

#include "tex-parser/head.h"

#include "postlist/math-postlist.h" /* for math_postlist_item */
#include "head.h"

/* ======================
 * math index open/close
 * ====================== */
math_index_t
math_index_open(const char *path, enum math_index_open_opt open_opt)
{
	math_index_t index;

	index = malloc(sizeof(struct math_index));
	sprintf(index->dir, "%s", path);

	index->open_opt = open_opt;

	if (open_opt == MATH_INDEX_WRITE) {
		mkdir_p(path);
		return index;

	} else if (open_opt == MATH_INDEX_READ_ONLY) {
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

/* ==========================
 * make path string function
 * ========================== */
struct _mk_path_str_arg {
	char **p;
	bool skip_first;
	int max;
	int cnt;
};

static LIST_IT_CALLBK(_mk_path_str)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct _mk_path_str_arg, pa_extra);

	if (arg->skip_first) {
		arg->skip_first = 0;
		goto next;
	}

	if (arg->max != 0) {
		if (arg->cnt == arg->max) {
			return LIST_RET_BREAK;
		}
		arg->cnt ++;
	}

	*(arg->p) += sprintf(*(arg->p), "/%s", trans_token(sp_nd->token_id));

next:
	LIST_GO_OVER;
}

bool
math_index_mk_path_str(struct subpath *sp, char *dest_path)
{
	char *p = dest_path;
	struct _mk_path_str_arg arg = {&p, 0, 0, 0};

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		p += sprintf(dest_path, "%s", GENER_PATH_NAME);
		arg.skip_first = 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_WILDCARD) {
		p += sprintf(dest_path, "%s", GENER_PATH_NAME);
		arg.skip_first = 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_NORMAL) {
		p += sprintf(dest_path, "%s", TOKEN_PATH_NAME);
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else {
		return 1;
	}

	return 0;
}

bool
math_index_mk_prefix_path_str(struct subpath *sp, int prefix_len,
                              char *dest_path)
{
	char *p = dest_path;
	struct _mk_path_str_arg arg = {&p, 0, prefix_len, 0};

	if (prefix_len > sp->n_nodes)
		return 1;

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		p += sprintf(dest_path, "%s", GENER_PATH_NAME);
		arg.skip_first = 1;
		arg.max -= 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_WILDCARD) {
		p += sprintf(dest_path, "%s", GENER_PATH_NAME);
		arg.skip_first = 1;
		arg.max -= 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_NORMAL) {
		p += sprintf(dest_path, "%s", PREFIX_PATH_NAME);
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else {
		return 1;
	}

	return 0;
}

int prefix_path_level(struct subpath *sp, int prefix_len)
{
	if (prefix_len > sp->n_nodes)
		return 1;

	return sp->n_nodes - prefix_len;
}

/* ================
 * write functions
 * ================ */
int write_posting_item_v2(const char *path,
                          struct math_posting_item_v2 *item)
{
	FILE *fh;
	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" MATH_POSTING_FNAME, path);

	fh = fopen(file_path, "a");
	if (fh == NULL)
		return -1;

	fwrite(item, 1, sizeof(struct math_posting_item_v2), fh);
	fclose(fh);

	return 0;
}

struct _set_pathinfo_endpoints_arg {
	uint32_t cnt, prefix_len;
	uint32_t leaf_id, subr_id;
};

static LIST_IT_CALLBK(_set_pathinfo_endpoints)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct _set_pathinfo_endpoints_arg, pa_extra);

	if (pa_now->now == pa_head->now) {
		arg->leaf_id = sp_nd->node_id;
	}

	if (arg->cnt >= arg->prefix_len - 1 ||
	    pa_now->now == pa_head->last) {
		arg->subr_id = sp_nd->node_id;
		return LIST_RET_BREAK;
	} else {
		arg->cnt ++;
		return LIST_RET_CONTINUE;
	}
}

struct _gener_path_match_args {
	uint32_t wild_node_id;
	uint32_t cur_test_path;
	uint64_t wild_leaves;
};

static LIST_IT_CALLBK(_gener_path_node_match)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(args, struct _gener_path_match_args, pa_extra);

	if (args->wild_node_id == sp_nd->node_id) {
		// printf("match #%u\n", args->cur_test_path);
		args->wild_leaves |= (1 << (args->cur_test_path - 1));
		return LIST_RET_BREAK;
	}

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(_gener_path_match)
{
	LIST_OBJ(struct subpath, test_sp, ln);
	P_CAST(args, struct _gener_path_match_args, pa_extra);

	args->cur_test_path = test_sp->leaf_id;

	if (test_sp->type == SUBPATH_TYPE_NORMAL)
		list_foreach(&test_sp->path_nodes, &_gener_path_node_match, pa_extra);

	LIST_GO_OVER;
}

static uint64_t get_wild_leaves(struct subpath *sp, struct subpaths subpaths)
{
	struct _gener_path_match_args args = {sp->leaf_id, 0, 0};
	list_foreach(&subpaths.li, &_gener_path_match, &args);

	return args.wild_leaves;
}

static int
write_pathinfo_v2(const char *path, struct subpath *sp,
                  uint32_t prefix_len, struct subpaths subpaths)
{
	struct _set_pathinfo_endpoints_arg arg = {0};

	FILE *fh;
	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" PATH_INFO_FNAME, path);

	fh = fopen(file_path, "a");
	if (fh == NULL)
		return -1;

	/* get end-point IDs (arg.leaf_id and arg.subr_id) */
	arg.cnt = 0;
	arg.prefix_len = prefix_len;
	list_foreach(&sp->path_nodes, &_set_pathinfo_endpoints, &arg);

	if (sp->type == SUBPATH_TYPE_NORMAL) {
		struct math_pathinfo_v2 pathinfo;

		pathinfo.op_hash = subpath_fingerprint(sp, prefix_len);
		pathinfo.lf_symb = sp->lf_symbol_id;
		pathinfo.subr_id = arg.subr_id;
		pathinfo.leaf_id = arg.leaf_id;

		/* write normal pathinfo */
		fwrite(&pathinfo, 1, sizeof(struct math_pathinfo_v2), fh);

	} else if (sp->type == SUBPATH_TYPE_GENERNODE) {
		struct math_pathinfo_gener_v2 pathinfo;

		pathinfo.op_hash = subpath_fingerprint(sp, prefix_len);
		pathinfo.tr_hash = sp->subtree_hash;
		pathinfo.subr_id = arg.subr_id;
		pathinfo.wild_leaves = get_wild_leaves(sp, subpaths);

		/* write gener pathinfo */
		fwrite(&pathinfo, 1, sizeof(struct math_pathinfo_gener_v2), fh);
	}

	fclose(fh);

#ifdef DEBUG_MATH_INDEX
	if (sp->type == SUBPATH_TYPE_GENERNODE)
	printf("written gener pathinfo @ %s: %u, %u, 0x%x, 0x%x, 0x%lx \n", path,
		arg.pathinfo.wild_id,
		arg.pathinfo.subr_id,
		arg.pathinfo.tr_hash,
		arg.pathinfo.op_hash,
		wild_leaves
	);
	else if (sp->type == SUBPATH_TYPE_NORMAL)
	printf("written normal pathinfo @ %s: %u, %u, %s, 0x%x \n", path,
		arg.pathinfo.leaf_id,
		arg.pathinfo.subr_id,
		trans_symbol(arg.pathinfo.lf_symb),
		arg.pathinfo.op_hash
	);
#endif
	return 0;
}


/* ======================
 * math path index steps
 * ====================== */

struct _index_path_arg {
	math_index_t     index;
	enum math_index_wr_opt opt;
	doc_id_t         docID;
	exp_id_t         expID;
	struct subpaths  subpaths;
	list             subpath_set;
	int              prefix_len;
};

uint32_t get_pathinfo_len(const char *path)
{
	FILE *fh;
	uint64_t ret;
	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" PATH_INFO_FNAME, path);

	fh = fopen(file_path, "r");
	if (fh == NULL)
		return 0;

	fseek(fh, 0, SEEK_END);
	ret = ftell(fh);
	fclose(fh);

	return (uint32_t)ret;
}

static LIST_IT_CALLBK(mkdir_and_setify)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	char *append = path;
	append += sprintf(append, "%s/", arg->index->dir);

	/* sanity check */
	if (sp->type != SUBPATH_TYPE_NORMAL &&
	    sp->type != SUBPATH_TYPE_GENERNODE) {
		LIST_GO_OVER;
	}

	/* only index prefix path under MATH_INDEX_PREFIX_ONLY */
	if (arg->opt == MATH_INDEX_PREFIX_ONLY &&
		sp->type == SUBPATH_TYPE_GENERNODE) {
		LIST_GO_OVER;
	}

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		int level = prefix_path_level(sp, arg->prefix_len);
#ifdef DEBUG_MATH_INDEX
		// printf("wildcard level = %u\n", level);
#endif
		if (level > MAX_WILDCARD_LEVEL) {
			/* this prefix gener path is lower than considered tree level */
			LIST_GO_OVER;
		}
	}

	if (math_index_mk_prefix_path_str(sp, arg->prefix_len, append)) {
		/* specified prefix_len is greater than actual path length */
		LIST_GO_OVER;
	}

#ifdef DEBUG_MATH_INDEX
	// printf("mkdir -p %s \n", path);
#endif
	mkdir_p(path);

	subpath_set_add(&arg->subpath_set, sp, arg->prefix_len);

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(set_write_to_index)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	char *append = path;
	uint32_t i;

	struct math_posting_item_v2  po_item;

	struct subpath tmp = *(ele->dup[0]);

	append += sprintf(append, "%s/", arg->index->dir);
	if (0 == math_index_mk_prefix_path_str(&tmp, arg->prefix_len, append)) {
		/* write posting item */
		po_item.exp_id       = arg->expID;
		po_item.n_lr_paths   = arg->subpaths.n_lr_paths;
		po_item.n_paths      = ele->dup_cnt + 1;
		po_item.doc_id       = arg->docID;
		po_item.pathinfo_pos = get_pathinfo_len(path); /* to be appended */
		write_posting_item_v2(path, &po_item);

		/* write n_paths number of pathinfo structure */
		for (i = 0; i <= ele->dup_cnt; i++) {
			struct subpath * sp = ele->dup[i];
			write_pathinfo_v2(path, sp, arg->prefix_len, arg->subpaths);
		}
	}

	LIST_GO_OVER;
}

int
math_index_add_tex(math_index_t index, doc_id_t docID, exp_id_t expID,
                   struct subpaths subpaths, enum math_index_wr_opt opt)
{
	struct _index_path_arg arg;
	arg.index = (math_index_t)index;
	arg.opt   = opt;
	arg.docID = docID;
	arg.expID = expID;
	arg.subpaths = subpaths;
	LIST_CONS(arg.subpath_set);

#ifdef DEBUG_MATH_INDEX
	printf("[math index adding %u subpaths]\n", subpaths.n_lr_paths);
#endif

	if (subpaths.n_lr_paths > MAX_MATH_PATHS) {
#ifdef DEBUG_MATH_INDEX
		printf("too many subpaths, abort.\n");
#endif
		return 1;
	}

	for (arg.prefix_len = 2;; arg.prefix_len++) {
		LIST_CONS(arg.subpath_set);
		list_foreach(&subpaths.li, &mkdir_and_setify, &arg);
#ifdef DEBUG_MATH_INDEX
		printf("subpath set at prefix_len = %d.\n", arg.prefix_len);
		subpath_set_print(&arg.subpath_set, stdout);
#endif

		if (arg.subpath_set.now == NULL) {
#ifdef DEBUG_MATH_INDEX
			printf("prefix index: break at prefix_len = %d.\n", arg.prefix_len);
#endif
			break;
		}

		list_foreach(&arg.subpath_set, &set_write_to_index, &arg);
		subpath_set_free(&arg.subpath_set);
	}

	return 0;
}

/* ===============================
 * probe math index posting list
 * =============================== */

int math_inex_probe_v2(const char* path, bool trans, bool gener, FILE *fh)
{
	int ret = 0;
	math_posting_t *po = math_posting_new_reader(path);

	/* start reading posting list (try to open file) */
	if (!math_posting_start(po)) {
		ret = 1;
		fprintf(stderr, "cannot start reading posting list.\n");
		goto free;
	}

	do {
		/* read posting list item */
		if (!gener) {
			struct math_postlist_item po_item;
			math_posting_read(po, &po_item);
			fprintf(fh, "doc#%u, exp#%u. %u lr_paths, %u paths {",
					po_item.doc_id, po_item.exp_id, po_item.n_lr_paths,
					po_item.n_paths);

			for (uint32_t i = 0; i < po_item.n_paths; i++) {
				struct math_postlist_item *p = &po_item;
				if (!trans) {
					fprintf(fh, "[%u ~ %u, symbol_%x, ophash_%x]",
						p->subr_id[i], p->leaf_id[i],
						p->lf_symb[i], p->op_hash[i]);
				} else {
					fprintf(fh, "[%u ~ %u, %s, op_hash%x]",
						p->subr_id[i], p->leaf_id[i],
						trans_symbol(p->lf_symb[i]), p->op_hash[i]);
				}

				if (i + 1 != po_item.n_paths)
					fprintf(fh, ", ");
			}
			fprintf(fh, "}");
		} else {
			struct math_postlist_gener_item po_item;
			math_posting_read_gener(po, &po_item);
			fprintf(fh, "doc#%u, exp#%u. %u lr_paths, %u paths {",
					po_item.doc_id, po_item.exp_id, po_item.n_lr_paths,
					po_item.n_paths);

			for (uint32_t i = 0; i < po_item.n_paths; i++) {
				struct math_postlist_gener_item *p = &po_item;
				fprintf(fh, "[%u ~ %lx, ophash_%x, symbol_%x]",
					p->subr_id[i], p->wild_leaves[i],
					p->op_hash[i], p->tr_hash[i]);

				if (i + 1 != po_item.n_paths)
					fprintf(fh, ", ");
			}
			fprintf(fh, "}");
		}

		/* finish probing this posting item */
		fprintf(fh, "\n");
	} while (math_posting_next(po));

free:
	math_posting_finish(po);
	math_posting_free_reader(po);

	return ret;
}
