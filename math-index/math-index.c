#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "list/list.h"
#include "tex-parser/tex-parser.h"

#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"  /* for token_id */
#include "tex-parser/gen-symbol.h" /* for symbol_id */
#include "tex-parser/trans.h"      /* for trans_token() */

#include "head.h"

/* ======================
 * math index open/close
 * ====================== */
math_index_t
math_index_open(const char *path, enum math_index_open_opt open_opt)
{
	math_index_t index;

	index = malloc(sizeof(struct math_index));
	sprintf(index->dir_gener, "%s/" GENER_PATH_NAME, path);
	sprintf(index->dir_token, "%s/" TOKEN_PATH_NAME, path);

	index->open_opt = open_opt;

	if (open_opt == MATH_INDEX_WRITE) {
		mkdir_p(path);
		mkdir_p(index->dir_gener);
		mkdir_p(index->dir_token);

		return index;

	} else if (open_opt == MATH_INDEX_READ_ONLY) {
		if (dir_exists(path) &&
		    dir_exists(index->dir_gener) &&
		    dir_exists(index->dir_token))
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
	bool skip_one;
};

static LIST_IT_CALLBK(_mk_path_str)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct _mk_path_str_arg, pa_extra);

	if (arg->skip_one) {
		arg->skip_one = 0;
		goto next;
	}

	*(arg->p) += sprintf(*(arg->p), "/%s", trans_token(sp_nd->token_id));

next:
	LIST_GO_OVER;
}

bool
math_index_mk_path_str(math_index_t index, struct subpath *sp,
                       char *dest_path)
{
	char *p = dest_path;
	struct _mk_path_str_arg arg = {&p, 0};

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		p += sprintf(dest_path, "%s", index->dir_gener);
		arg.skip_one = 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_WILDCARD) {
		p += sprintf(dest_path, "%s", index->dir_gener);
		arg.skip_one = 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_NORMAL) {
		p += sprintf(dest_path, "%s", index->dir_token);
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else {
		return 1;
	}

	return 0;
}

/* ================
 * write functions
 * ================ */

static int
write_pathinfo_payload(const char *path, struct math_pathinfo *pathinfo)
{
	FILE *fh;
	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" PATH_INFO_FNAME, path);

	fh = fopen(file_path, "a");
	if (fh == NULL)
		return -1;

	fwrite(pathinfo, 1, sizeof(struct math_pathinfo), fh);
	fclose(fh);

	return 0;
}

static void
write_pathinfo_head(const char *path, struct math_pathinfo_pack* pack)
{
	FILE *fh;
	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" PATH_INFO_FNAME, path);

	fh = fopen(file_path, "a");
	if (fh == NULL)
		return;

	fwrite(pack, 1, sizeof(struct math_pathinfo_pack), fh);
	fclose(fh);
}

static int
wirte_posting_item(const char *path, struct math_posting_item *po_item)
{
	FILE *fh;
	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" MATH_POSTING_FNAME, path);

	fh = fopen(file_path, "a");
	if (fh == NULL)
		return -1;

	fwrite(po_item, 1, sizeof(struct math_posting_item), fh);
	fclose(fh);

	return 0;
}

/* ======================
 * math path index steps
 * ====================== */

struct _index_path_arg {
	math_index_t   index;
	doc_id_t       docID;
	exp_id_t       expID;
	list           subpath_set;
	uint32_t       n_lr_paths;
};

static LIST_IT_CALLBK(path_index_step1)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";

	if (math_index_mk_path_str(arg->index, sp, path)) {
		LIST_GO_OVER;
	}

	mkdir_p(path);
	subpath_set_add(&arg->subpath_set, sp);

	LIST_GO_OVER;
}

static uint64_t pathinfo_len(const char *path)
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

	return ret;
}

static LIST_IT_CALLBK(path_index_step2)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";

	struct math_posting_item  po_item;
	struct math_pathinfo_pack pathinfo_hd;

	struct subpath tmp;
	tmp.type = ele->dup[0]->type;
	tmp.path_nodes = ele->dup[0]->path_nodes;

	if (0 == math_index_mk_path_str(arg->index, &tmp, path)) {
		/* wirte posting item */
		po_item.doc_id = arg->docID;
		po_item.exp_id = arg->expID;
		po_item.pathinfo_pos = pathinfo_len(path);
		wirte_posting_item(path, &po_item);

		/* wirte pathinfo head */
		pathinfo_hd.n_paths = ele->dup_cnt + 1;
		pathinfo_hd.n_lr_paths = arg->n_lr_paths;
		write_pathinfo_head(path, &pathinfo_hd);
	}

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(path_index_step3)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	struct math_pathinfo info = {sp->path_id, sp->ge_hash, sp->fr_hash};

	if (math_index_mk_path_str(arg->index, sp, path)) {
		LIST_GO_OVER;
	}

	if (0 != write_pathinfo_payload(path, &info)) {
		fprintf(stderr, "cannot write path info @%s\n", path);
		LIST_GO_OVER;
	}

	LIST_GO_OVER;
}

int
math_index_add_tex(math_index_t index, doc_id_t docID,
                   exp_id_t expID, struct subpaths subpaths)
{
	struct _index_path_arg arg;
	arg.index = (math_index_t)index;
	arg.docID = docID;
	arg.expID = expID;
	arg.n_lr_paths = subpaths.n_lr_paths;
	LIST_CONS(arg.subpath_set);

	list_foreach(&subpaths.li, &path_index_step1, &arg);
	list_foreach(&arg.subpath_set, &path_index_step2, &arg);
	list_foreach(&subpaths.li, &path_index_step3, &arg);

	return 0;
}

/* ===============================
 * probe math index posting list
 * =============================== */

int math_inex_probe(const char* path, bool trans, FILE *fh)
{
	int ret = 0;
	uint32_t i;
	uint64_t pos;
	math_posting_t *po;

	struct math_posting_item *po_item;
	struct math_pathinfo_pack *pathinfo_pack;
	struct math_pathinfo *pathinfo;

	/* allocate memory for posting reader */
	po = math_posting_new_reader(NULL, path);

	/* start reading posting list (try to open file) */
	if (!math_posting_start(po)) {
		ret = 1;
		fprintf(stderr, "cannot start reading posting list.\n");
		goto free;
	}

	while (math_posting_next(po)) {
		/* read posting list item */
		po_item = math_posting_current(po);
		pos = po_item->pathinfo_pos;
		fprintf(fh, "doc#%u, exp#%u pathinfo@%lu;",
		        po_item->doc_id, po_item->exp_id, pos);

		/* then read path info items */
		if (NULL == (pathinfo_pack = math_posting_pathinfo(po, pos))) {
			ret = 1;
			fprintf(stderr, "\n");
			fprintf(stderr, "fails to read math posting pathinfo.\n");
			goto free;
		}

		/* upon success, print path info items */
		fprintf(fh, " %u lr_paths, {", pathinfo_pack->n_lr_paths);
		for (i = 0; i < pathinfo_pack->n_paths; i++) {
			pathinfo = pathinfo_pack->pathinfo + i;
			if (!trans) {
				fprintf(fh, "[%u %x %x]",
				        pathinfo->path_id, pathinfo->lf_symb,
				        pathinfo->fr_hash);
			} else {
				fprintf(fh, "[%u %s %x]",
				        pathinfo->path_id,
				        trans_symbol(pathinfo->lf_symb),
				        pathinfo->fr_hash);
			}

			if (i + 1 != pathinfo_pack->n_paths)
				fprintf(fh, ", ");
		}
		fprintf(fh, "}");

		/* finish probing this posting item */
		fprintf(fh, "\n");
	}

	/* a little double-check */
	po_item = math_posting_current(po);
	assert(po_item->doc_id == 0);

free:
	math_posting_finish(po);
	math_posting_free_reader(po);

	return ret;
}
