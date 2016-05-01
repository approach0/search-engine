#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "list/list.h"
#include "dir-util/dir-util.h"
#include "tex-parser/tex-parser.h"

#include "tex-parser/config.h"
#include "tex-parser/gen-token.h"  /* for token_id */
#include "tex-parser/gen-symbol.h" /* for symbol_id */
#include "tex-parser/trans.h"      /* for trans_token() */

#include "math-index.h"
#include "subpath-set.h"
#include "config.h"

typedef struct {
	enum math_index_open_opt open_opt;
	char dir_gener[MAX_DIR_PATH_NAME_LEN];
	char dir_token[MAX_DIR_PATH_NAME_LEN];
} _math_index_t;

typedef struct {
	FILE *id_list;
	FILE *path_info;
} _math_posting_t;

struct index_path_arg {
	_math_index_t *index;
	uint32_t       n_lr_paths;
	doc_id_t       docID;
	exp_id_t       expID;
	list           subpath_set;
};

math_index_t
math_index_open(const char *path, enum math_index_open_opt open_opt)
{
	_math_index_t *index;

	index = malloc(sizeof(_math_index_t));
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

static LIST_IT_CALLBK(mk_path_str)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(p, char*, pa_extra);

	*p += sprintf(*p, "/%s", trans_token(sp_nd->token_id));

	LIST_GO_OVER;
}

bool math_index_mk_path_str(list *path_nodes, enum subpath_type type,
                            char *dir_gener, char *dir_token, char *dest_path)
{
	char *p = dest_path;

	if (type == SUBPATH_TYPE_GENERNODE)
		p += sprintf(dest_path, "%s", dir_gener);
	else if (type  == SUBPATH_TYPE_NORMAL)
		p += sprintf(dest_path, "%s", dir_token);
	else
		return 1;

	list_foreach(path_nodes, &mk_path_str, &p);
	return 0;
}

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

static uint64_t read_pathinfo_fpos(const char *path)
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

static LIST_IT_CALLBK(mk_dir_and_subpath_set)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	struct math_posting_item po_item;

	if (math_index_mk_path_str(&sp->path_nodes, sp->type,
	                       arg->index->dir_gener, arg->index->dir_token,
                           path)) {
		LIST_GO_OVER;
	}

	mkdir_p(path);

	po_item.doc_id = arg->docID;
	po_item.exp_id = arg->expID;
	po_item.pathinfo_pos = read_pathinfo_fpos(path);
	subpath_set_add(&arg->subpath_set, sp, &po_item);

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(index_pathinfo_head)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(arg, struct index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	struct math_pathinfo_pack head;

	if (0 == math_index_mk_path_str(&ele->path_nodes, ele->subpath_type,
	                       arg->index->dir_gener, arg->index->dir_token,
                           path)) {
		head.n_paths = ele->dup_cnt + 1;
		head.n_lr_paths = arg->n_lr_paths;
		write_pathinfo_head(path, &head);
	}

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(index_pathinfo_payload)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	struct math_pathinfo info = {sp->path_id, sp->ge_hash, sp->fr_hash};

	if (math_index_mk_path_str(&sp->path_nodes, sp->type,
	                       arg->index->dir_gener, arg->index->dir_token,
                           path)) {
		LIST_GO_OVER;
	}

	if (0 != write_pathinfo_payload(path, &info)) {
		fprintf(stderr, "cannot write path info @%s\n", path);
		LIST_GO_OVER;
	}

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(index_posting_item)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(arg, struct index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";

	if (0 == math_index_mk_path_str(&ele->path_nodes, ele->subpath_type,
	                       arg->index->dir_gener, arg->index->dir_token,
                           path)) {
		wirte_posting_item(path, &ele->to_write);
	}

	LIST_GO_OVER;
}

int
math_index_add_tex(math_index_t index, doc_id_t docID,
                   exp_id_t expID, struct subpaths subpaths)
{
	struct index_path_arg arg;
	arg.index = (_math_index_t*)index;
	arg.docID = docID;
	arg.expID = expID;
	arg.n_lr_paths = subpaths.n_lr_paths;
	LIST_CONS(arg.subpath_set);

	list_foreach(&subpaths.li, &mk_dir_and_subpath_set, &arg);
	subpath_set_print(&arg.subpath_set, stdout);
	list_foreach(&arg.subpath_set, &index_pathinfo_head, &arg);
	list_foreach(&subpaths.li, &index_pathinfo_payload, &arg);
	list_foreach(&arg.subpath_set, &index_posting_item, &arg);

	return 0;
}

int math_inex_probe(const char* path, bool trans, FILE *fh_pri)
{
	int ret = 0;
	pathinfo_num_t cnt;
	FILE *fh_posting = NULL, *fh_pathinfo = NULL;
	struct math_posting_item po_item;
	struct math_pathinfo_pack pathinfo_head;
	struct math_pathinfo pathinfo;
	char file_path[MAX_DIR_PATH_NAME_LEN];

	sprintf(file_path, "%s/" MATH_POSTING_FNAME, path);
	fh_posting = fopen(file_path, "r");
	if (fh_posting == NULL) {
		ret = 1;
		goto free;
	}

	sprintf(file_path, "%s/" PATH_INFO_FNAME, path);
	fh_pathinfo = fopen(file_path, "r");
	if (fh_pathinfo == NULL) {
		ret = 1;
		goto free;
	}

	while (1 == fread(&po_item, sizeof(struct math_posting_item),
	                  1, fh_posting)) {
		fprintf(fh_pri, "doc#%u, exp#%u pos@%lu;",
		        po_item.doc_id, po_item.exp_id, po_item.pathinfo_pos);

		fseek(fh_pathinfo, po_item.pathinfo_pos, SEEK_SET);
		if (1 == fread(&pathinfo_head, sizeof(struct math_pathinfo_pack),
		               1, fh_pathinfo)) {
			fprintf(fh_pri, " {");
			fprintf(fh_pri, "lr=%u; ", pathinfo_head.n_lr_paths);

			for (cnt = 0; cnt < pathinfo_head.n_paths; cnt++) {
				if (1 != fread(&pathinfo, sizeof(struct math_pathinfo),
				       1, fh_pathinfo))
					break;

				if (!trans) {
					fprintf(fh_pri, "[%u %x %x]",
					        pathinfo.path_id, pathinfo.lf_symb,
					        pathinfo.fr_hash);
				} else {
					fprintf(fh_pri, "[%u %s %x]",
					        pathinfo.path_id,
					        trans_symbol(pathinfo.lf_symb),
					        pathinfo.fr_hash);
				}

				if (cnt + 1 != pathinfo_head.n_paths)
					fprintf(fh_pri, ", ");
			}

			fprintf(fh_pri, "}");
		}

		fprintf(fh_pri, "\n");
	}

free:
	if (fh_posting)
		fclose(fh_posting);

	if (fh_pathinfo)
		fclose(fh_pathinfo);

	return ret;
}
