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
	bool skip_one;
	int max;
	int cnt;
};

static LIST_IT_CALLBK(_mk_path_str)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct _mk_path_str_arg, pa_extra);

	if (arg->skip_one) {
		arg->skip_one = 0;
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

void
math_index_mk_base_path_str(struct subpath *sp, char *dest_path)
{
	char *p = dest_path;
	struct _mk_path_str_arg arg = {&p, 0, 0, 0};
	list_foreach(&sp->path_nodes, &_mk_path_str, &arg);
}

bool
math_index_mk_path_str(struct subpath *sp, char *dest_path)
{
	char *p = dest_path;
	struct _mk_path_str_arg arg = {&p, 0, 0, 0};

	if (sp->type == SUBPATH_TYPE_GENERNODE) {
		p += sprintf(dest_path, "%s", GENER_PATH_NAME);
		arg.skip_one = 1;
		list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

	} else if (sp->type  == SUBPATH_TYPE_WILDCARD) {
		p += sprintf(dest_path, "%s", GENER_PATH_NAME);
		arg.skip_one = 1;
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

	p += sprintf(dest_path, "%s", "prefix");
	list_foreach(&sp->path_nodes, &_mk_path_str, &arg);

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
write_posting_item(const char *path, struct math_posting_item *po_item)
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

static int
write_posting_item_v2(const char *path,
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

struct _set_pathinfo_v2_arg {
	uint32_t                 prefix_len;
	uint32_t                 cnt;
	struct math_pathinfo_v2  pathinfo;
};

static LIST_IT_CALLBK(_set_pathinfo_v2)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct _set_pathinfo_v2_arg, pa_extra);
	
	if (pa_now->now == pa_head->now)
		arg->pathinfo.leaf_id = sp_nd->node_id;

	if (arg->cnt >= arg->prefix_len - 1 ||
	    pa_now->now == pa_head->last) {
		arg->pathinfo.subr_id = sp_nd->node_id;
		return LIST_RET_BREAK;
	} else {
		arg->cnt ++;
		return LIST_RET_CONTINUE;
	}
}

static int
write_pathinfo_v2(const char *path, struct subpath *sp,
                  uint32_t prefix_len)
{
	struct _set_pathinfo_v2_arg arg;
	FILE *fh;
	char file_path[MAX_DIR_PATH_NAME_LEN];
	sprintf(file_path, "%s/" PATH_INFO_FNAME, path);

	fh = fopen(file_path, "a");
	if (fh == NULL)
		return -1;

	arg.pathinfo.lf_symb = sp->lf_symbol_id;
	arg.cnt = 0;
	arg.prefix_len = prefix_len;

	list_foreach(&sp->path_nodes, &_set_pathinfo_v2, &arg);

#ifdef DEBUG_MATH_INDEX
	printf("writing pathinfo @ %s: %u, %u, %u \n", path,
		arg.pathinfo.leaf_id,
		arg.pathinfo.subr_id,
		arg.pathinfo.lf_symb
	);
#endif

	fwrite(&arg.pathinfo, 1, sizeof(arg.pathinfo), fh);
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
	int            prefix_len;
};

static LIST_IT_CALLBK(path_index_step1)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	char *append = path;

	append += sprintf(append, "%s/", arg->index->dir);
	if (math_index_mk_path_str(sp, append)) {
		LIST_GO_OVER;
	}

	mkdir_p(path);

	subpath_set_add(&arg->subpath_set, sp, 0);

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(path_index_step4)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	char *append = path;
	
	if (sp->type != SUBPATH_TYPE_NORMAL) {
		LIST_GO_OVER;
	}

	append += sprintf(append, "%s/", arg->index->dir);
	if (math_index_mk_prefix_path_str(sp, arg->prefix_len, append)) {
		LIST_GO_OVER;
	}

	//printf("mkdir -p %s \n", path);
	mkdir_p(path);

	subpath_set_add(&arg->subpath_set, sp, arg->prefix_len);

	LIST_GO_OVER;
}

static uint32_t pathinfo_len(const char *path)
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

static LIST_IT_CALLBK(path_index_step2)
{
	LIST_OBJ(struct subpath_ele, ele, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	char *append = path;

	struct math_posting_item  po_item;
	struct math_pathinfo_pack pathinfo_hd;

	struct subpath tmp = *(ele->dup[0]);

	append += sprintf(append, "%s/", arg->index->dir);
	if (0 == math_index_mk_path_str(&tmp, append)) {
		/* write posting item */
		po_item.doc_id = arg->docID;
		po_item.exp_id = arg->expID;
		po_item.pathinfo_pos = pathinfo_len(path);
#ifdef DEBUG_MATH_INDEX
		printf("write item(docID=%u, expID=%u, pos=%u) @ %s.\n",
		       po_item.doc_id, po_item.exp_id, po_item.pathinfo_pos, path);
#endif
		write_posting_item(path, &po_item);

		/* write pathinfo head */
		pathinfo_hd.n_paths = ele->dup_cnt + 1;
		pathinfo_hd.n_lr_paths = arg->n_lr_paths;
#ifdef DEBUG_MATH_INDEX
		printf("write pathinfo head(n_paths=%u, n_lr_paths=%u) @ %s.\n",
		       pathinfo_hd.n_paths, pathinfo_hd.n_lr_paths, path);
#endif
		write_pathinfo_head(path, &pathinfo_hd);
	}

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(path_index_step5)
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
		po_item.n_lr_paths   = arg->n_lr_paths;
		po_item.n_paths      = ele->dup_cnt + 1;
		po_item.doc_id       = arg->docID;
		po_item.pathinfo_pos = pathinfo_len(path);
		write_posting_item_v2(path, &po_item);

		/* write n_paths number of pathinfo structure */
		for (i = 0; i <= ele->dup_cnt; i++) {
			struct subpath * sp = ele->dup[i];
			write_pathinfo_v2(path, sp, arg->prefix_len);
		}
	}

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(path_index_step3)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(arg, struct _index_path_arg, pa_extra);
	char path[MAX_DIR_PATH_NAME_LEN] = "";
	char *append = path;
	struct math_pathinfo info = {sp->path_id, sp->ge_hash, sp->fr_hash};

	append += sprintf(append, "%s/", arg->index->dir);
	if (math_index_mk_path_str(sp, append)) {
		LIST_GO_OVER;
	}

#ifdef DEBUG_MATH_INDEX
	printf("write pathinfo item(pathID=%u, ge_hash/symbol=%x) @ %s.\n",
	       info.path_id, info.lf_symb, path);
#endif
	if (0 != write_pathinfo_payload(path, &info)) {
		fprintf(stderr, "cannot write path info @%s\n", path);
		LIST_GO_OVER;
	}

	LIST_GO_OVER;
}

int
math_index_add_tex(math_index_t index, doc_id_t docID, exp_id_t expID,
                   struct subpaths subpaths, enum math_index_wr_opt opt)
{
	struct _index_path_arg arg;
	arg.index = (math_index_t)index;
	arg.docID = docID;
	arg.expID = expID;
	arg.n_lr_paths = subpaths.n_lr_paths;
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

#ifdef DEBUG_MATH_INDEX
	printf("path index step 1 (adding into subpath set):\n");
#endif

	list_foreach(&subpaths.li, &path_index_step1, &arg);

#ifdef DEBUG_MATH_INDEX
	printf("subpath set:\n");
	subpath_set_print(&arg.subpath_set, stdout);

	printf("path index step 2 (write post item and pathinfo head)...\n");
#endif
	list_foreach(&arg.subpath_set, &path_index_step2, &arg);

#ifdef DEBUG_MATH_INDEX
	printf("path index step 3 (write pathinfo payload)...\n");
#endif
	list_foreach(&subpaths.li, &path_index_step3, &arg);

#ifdef DEBUG_MATH_INDEX
	printf("deleting subpath set...\n");
#endif
	subpath_set_free(&arg.subpath_set);

	if (opt == MATH_INDEX_WO_PREFIX)
		goto skip_prefix;

	for (arg.prefix_len = 2;; arg.prefix_len++) {
		LIST_CONS(arg.subpath_set);
		list_foreach(&subpaths.li, &path_index_step4, &arg);
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

		list_foreach(&arg.subpath_set, &path_index_step5, &arg);
		subpath_set_free(&arg.subpath_set);
	}

skip_prefix:
	return 0;
}

/* ===============================
 * probe math index posting list
 * =============================== */

int math_inex_probe(const char* path, bool trans, FILE *fh)
{
	int ret = 0;
	uint32_t i;
	uint32_t pos;
	math_posting_t *po;

	struct math_posting_item *po_item;
	struct math_pathinfo_pack *pathinfo_pack;
	struct math_pathinfo *pathinfo;

	/* allocate memory for posting reader */
	po = math_posting_new_reader(path);

	/* start reading posting list (try to open file) */
	if (!math_posting_start(po)) {
		ret = 1;
		fprintf(stderr, "cannot start reading posting list.\n");
		goto free;
	}

	/* assume first item must exists, which is actually true. */
	do {
		/* read posting list item */
		po_item = math_posting_current(po);
		pos = po_item->pathinfo_pos;
		fprintf(fh, "doc#%u, exp#%u pathinfo@%u;",
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
	} while (math_posting_next(po));

free:
	math_posting_finish(po);
	math_posting_free_reader(po);

	return ret;
}

int math_inex_probe_v2(const char* path, bool trans, FILE *fh)
{
	int ret = 0;
	uint32_t i;
	uint32_t pos;
	math_posting_t *po;

	struct math_posting_item_v2 *po_item;

	/* allocate memory for posting reader */
	po = math_posting_new_reader(path);

	/* start reading posting list (try to open file) */
	if (!math_posting_start(po)) {
		ret = 1;
		fprintf(stderr, "cannot start reading posting list.\n");
		goto free;
	}

	do {
		/* read posting list item */
		po_item = (struct math_posting_item_v2*)math_posting_current(po);
		pos = po_item->pathinfo_pos;
		fprintf(fh, "doc#%u, exp#%u pathinfo@%u;" " %u lr_paths, %u paths {",
		        po_item->doc_id, po_item->exp_id, pos, po_item->n_lr_paths,
		        po_item->n_paths);

		/* then read path info items */
		struct math_pathinfo_v2 pathinfo[MAX_MATH_PATHS];
		if (math_posting_pathinfo_v2(po, pos, po_item->n_paths, pathinfo)) {
			ret = 1;
			fprintf(stderr, "\n");
			fprintf(stderr, "fails to read math posting pathinfo.\n");
			goto free;
		}

		#ifdef DEBUG_MATH_INDEX
		printf("\nreading pathinfo @ %s:pos%u: %u, %u, %u \n", path, pos,
			pathinfo[0].leaf_id,
			pathinfo[0].subr_id,
			pathinfo[0].lf_symb
		);
		#endif

		/* upon success, print path info items */
		for (i = 0; i < po_item->n_paths; i++) {
			struct math_pathinfo_v2 *p = pathinfo + i;
			if (!trans) {
				fprintf(fh, "[%u ~ %u, %x]", p->subr_id, p->leaf_id,
				                             p->lf_symb);
			} else {
				fprintf(fh, "[%u ~ %u, %s]", p->subr_id, p->leaf_id,
				                             trans_symbol(p->lf_symb));
			}

			if (i + 1 != po_item->n_paths)
				fprintf(fh, ", ");
		}
		fprintf(fh, "}");

		/* finish probing this posting item */
		fprintf(fh, "\n");
	} while (math_posting_next(po));

free:
	math_posting_finish(po);
	math_posting_free_reader(po);

	return ret;
}
