#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

struct codec_buf_struct_info *math_codec_info()
{
	struct codec_buf_struct_info *info;
	info = codec_buf_struct_info_alloc(5, sizeof(struct math_invlist_item));

#define SET_FIELD_INFO(_idx, _name, _codec) \
	info->field_info[_idx] = FIELD_INFO(struct math_invlist_item, _name, _codec)

	SET_FIELD_INFO(FI_DOCID, docID, CODEC_FOR); // CODEC_FOR_DELTA);
	SET_FIELD_INFO(FI_SECID, secID, CODEC_FOR);
	SET_FIELD_INFO(FI_SECT_WIDTH, sect_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_ORIG_WIDTH, orig_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_OFFSET, symbinfo_offset, CODEC_FOR); // CODEC_FOR_DELTA);

	return info;
}

math_index_t
math_index_open(const char *path, const char *mode)
{
	math_index_t index;

	index = malloc(sizeof(struct math_index));

	sprintf(index->dir, "%s", path);
	sprintf(index->mode, "%s", mode);

	index->dict = strmap_new();
	index->cinfo = math_codec_info();
	index->memo_usage = sizeof(struct math_index);

	{  /* read the previous N value */
		char path[MAX_PATH_LEN];
		sprintf(path, "%s/%s.bin", index->dir, MINDEX_N_FNAME);
		FILE *fh_N = fopen(path, "r");
		if (NULL == fh_N) {
			index->N = 0;
		} else {
			fread(&index->N, 1, sizeof index->N, fh_N);
			fclose(fh_N);
		}
	}

	if (mode[0] == 'w') {
		mkdir_p(path);
		return index;

	} else if (mode[0] == 'r') {
		if (dir_exists(path))
			return index;
	}

	/* return NULL for invalid mode string */
	free(index);
	return NULL;
}

void math_index_flush(math_index_t index)
{
	foreach (iter, strmap, index->dict) {
		struct math_invlist_entry *entry = iter->cur->value;

		if (entry->writer &&
		    index->mode[0] == 'w') /* flush only in write mode */
			(void)invlist_writer_flush(entry->writer);

		if (entry->fh_symbinfo)
			fflush(entry->fh_symbinfo);

		if (entry->fh_pf)
			fflush(entry->fh_pf);
	}

	/* write the current N value if it is in write mode */
	if (index->mode[0] == 'w') {
		char path[MAX_PATH_LEN];
		sprintf(path, "%s/%s.bin", index->dir, MINDEX_N_FNAME);
		FILE *fh_N = fopen(path, "w");
		if (fh_N) {
			fwrite(&index->N, 1, sizeof index->N, fh_N);
			fclose(fh_N);
		}
	}
}

void math_index_close(math_index_t index)
{
	if (index->mode[0] == 'w')
		math_index_flush(index);

	foreach (iter, strmap, index->dict) {
		struct math_invlist_entry *entry = iter->cur->value;

		if (entry->writer)
			invlist_iter_free(entry->writer);

		if (entry->fh_symbinfo)
			fclose(entry->fh_symbinfo);

		if (entry->fh_pf)
			fclose(entry->fh_pf);

		if (entry->invlist)
			invlist_free(entry->invlist);

		free(entry);
	}

	/* free member structure */
	strmap_free(index->dict);
	codec_buf_struct_info_free(index->cinfo);
	free(index);
}

void math_index_print(math_index_t index)
{
	printf("[math index] %s (memo_usage=%luKB, N=%u, open mode: %s)\n",
		index->dir, index->memo_usage / 1024, index->N, index->mode);

	int max = 100, cnt = 0;
	foreach (iter, strmap, index->dict) {
		struct math_invlist_entry *entry = iter->cur->value;
		if (entry->invlist->type == INVLIST_TYPE_ONDISK) {
			printf("[on-disk] %s ", iter->cur->keystr);
		} else {
			printf("[in-memo] %p ", iter->cur->keystr);
		}
		printf(" (pf = %u)\n", entry->pf);

		invlist_print_as_decoded_ints(entry->invlist);

		if (cnt > max) break;
		cnt ++;
	}
}

static void remove_wildcards(linkli_t *set)
{
	foreach (iter, li, *set) {
		struct subpath_ele *ele = li_entry(ele, iter->cur, ln);
		if (ele->dup[0]->type == SUBPATH_TYPE_WILDCARD) {
			li_iter_remove(iter, set);
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

static struct math_invlist_entry
read_invlist_entry(struct codec_buf_struct_info *cinfo, const char *path)
{
	struct math_invlist_entry entry = {0};
	/* make path names */
	char invlist_path[MAX_PATH_LEN];
	char symbinfo_path[MAX_PATH_LEN];
	char pf_path[MAX_PATH_LEN];
	sprintf(invlist_path, "%s/%s", path, MINVLIST_FNAME);
	sprintf(symbinfo_path, "%s/%s.bin", path, SYMBINFO_FNAME);
	sprintf(pf_path, "%s/%s.bin", path, PATHFREQ_FNAME);

	/* open inverted list on disk */
	entry.invlist = invlist_open(invlist_path, MATH_INDEX_BLK_LEN, cinfo);

	/* set writer to NULL initially */
	entry.writer = NULL; /* only needed to cache writing */

	/* open symbinfo file */
	entry.fh_symbinfo = fopen(symbinfo_path, "a");
	assert(entry.fh_symbinfo != NULL);
	entry.offset = 0;

	/* set path frequency */
	{ /* first read out value if there exists a file */
		FILE *fh = fopen(pf_path, "r");
		if (NULL != fh) {
			fread(&entry.pf, 1, sizeof entry.pf, fh);
			fclose(fh);
		} else {
			entry.pf = 0; /* first-time touch base */
		}
	}
	/* then open for writing for later value updation */
	entry.fh_pf = fopen(pf_path, "w");

	return entry;
}

static size_t
memo_usage_per_entry(struct codec_buf_struct_info *cinfo, size_t str_len)
{
	size_t tot_sz = 0;

	tot_sz += sizeof(struct strmap_entry) + str_len;
	tot_sz += sizeof(datrie_state_t) * 2;

	tot_sz += sizeof(struct math_invlist_entry);
	tot_sz += sizeof(struct invlist_iterator);
	tot_sz += codec_buf_space(MATH_INDEX_BLK_LEN, cinfo);

	return tot_sz;
}

static void cache_append_invlist(math_index_t index, char *path,
	struct subpath_ele *ele, doc_id_t docID, exp_id_t expID, uint32_t width)
{
	struct math_invlist_entry *entry = NULL;
	strmap_t dict = index->dict;
	char *key_path = path + strlen(index->dir);

	/* add path into dictionary if this path does not exist */
	if (NULL == (entry = strmap_lookup(dict, key_path))) {
		/* create dictionary entry */
		entry = malloc(sizeof *entry);
		dict[[key_path]] = entry;

		/* open invlist and entry metadata */
		*entry = read_invlist_entry(index->cinfo, path);

		/* open invlist writer */
		entry->writer = invlist_writer(entry->invlist);

		/* update memory usage */
		index->memo_usage += memo_usage_per_entry(index->cinfo, strlen(path));
	}

	/* append invert list item */
	struct math_invlist_item item;
	struct symbinfo symbinfo;

	for (int i = 0; i < ele->n_sects; i++) {
		/* prepare sector tree structure */
		struct sector_tree secttr = ele->secttr[i];
		item.docID = docID;

		item.expID = expID;
		item.sect_root = secttr.rootID;

		item.sect_width = secttr.width;
		item.orig_width = width;
		item.symbinfo_offset = entry->offset;

		/* write sector tree structure */
		size_t flush_sz = invlist_writer_write(entry->writer, &item);
		(void)flush_sz;

		/* prepare symbinfo structure */
		symbinfo.ophash = secttr.ophash;
		symbinfo.n_splits = MIN(ele->n_splits[i], MAX_INDEX_EXP_SYMBOL_SPLITS);
		for (int j = 0; j < symbinfo.n_splits; j++) {
			symbinfo.symbol[j] = ele->symbol[i][j];
			symbinfo.splt_w[j] = ele->splt_w[i][j];
		}

		/* write symbinfo structure, update offset by written bytes */
		entry->offset += fwrite(&symbinfo, 1, sizeof symbinfo,
		                        entry->fh_symbinfo);

		/* update frequency statistics */
		entry->pf ++;
		rewind(entry->fh_pf);
		fwrite(&entry->pf, 1, sizeof entry->pf, entry->fh_pf);

		index->N ++;
	}
}

static void add_subpath_set(math_index_t index, linkli_t set,
                            doc_id_t docID, exp_id_t expID, uint32_t width)
{
	const char *root_path = index->dir;
	foreach (iter, li, set) {
		char path[MAX_DIR_PATH_NAME_LEN] = "";
		char *p = path;
		p += sprintf(p, "%s/", root_path);

		struct subpath_ele *ele = li_entry(ele, iter->cur, ln);
		struct subpath *sp = ele->dup[0];

		if (0 != mk_path_str(sp, ele->prefix_len, p)) {
			/* specified prefix_len is greater than actual path length */
			continue;
		}

		/* make directory */
#ifdef DEBUG_SUBPATH_SET
		printf("mkdir -p %s\n", path);
#endif
		mkdir_p(path);

		cache_append_invlist(index, path, ele, docID, expID, width);
	}
}

int math_index_add(math_index_t index, doc_id_t docID, exp_id_t expID,
                   struct subpaths subpaths)
{
	if (index->mode[0] != 'w') {
		prerr("math_index_add needs writing permission.\n");
		return 1;
	}

	if (subpaths.n_lr_paths > MAX_MATH_PATHS) {
		prerr("too many subpaths (%lu > %lu), abort.\n",
			subpaths.n_lr_paths, MAX_MATH_PATHS);
		return 1;
	}

	linkli_t set = subpath_set(subpaths, SUBPATH_SET_DOC);

	/* for indexing, we do not accept wildcards */
	remove_wildcards(&set);

#ifdef DEBUG_SUBPATH_SET
	printf("subpath set (size=%d)\n", li_size(set));
	print_subpath_set(set);
#endif

	add_subpath_set(index, set, docID, expID, subpaths.n_lr_paths);

	li_free(set, struct subpath_ele, ln, free(e));
	return 0;
}

struct math_index_load_arg {
	math_index_t index;
	size_t       limit_sz;
};

static enum ds_ret
dir_search_callbk(const char* path, const char *srchpath,
                  uint32_t level, void *args_)
{
	/* read arguments and cast */
	P_CAST(args, struct math_index_load_arg, args_);
	struct math_index *index = args->index;
	strmap_t dict = index->dict;
	char *key_path = (char *)srchpath + 1; /* omit the "." */

	/* is this directory empty? */
	char test_path[MAX_PATH_LEN];
	sprintf(test_path, "%s/%s.bin", path, MINVLIST_FNAME);
	if (!file_exists(test_path) /* no inverted list here */)
		return DS_RET_CONTINUE;

	/* unexpected duplication check */
	if (NULL != strmap_lookup(dict, key_path)) {
		prerr("key `%s' already exists when loading math index.", key_path);
		return DS_RET_CONTINUE;
	}

	/* allocate entry */
	struct math_invlist_entry *entry;
	entry = malloc(sizeof *entry);
	dict[[key_path]] = entry;

	/* open invlist and entry metadata */
	*entry = read_invlist_entry(index->cinfo, path);

	/* uncomment to test */
	//invlist_print_as_decoded_ints(entry->invlist);

	if (index->memo_usage > args->limit_sz) {
		prinfo("math index cache size reaches limit, stop caching.");
		return DS_RET_STOP_ALLDIR;
	} else {
		return DS_RET_CONTINUE;
	}
}

math_index_t math_index_load(const char *path, size_t limit_sz)
{
	math_index_t index = math_index_open(path, "r");
	if (NULL == index)
		return NULL;

	struct math_index_load_arg args = {index, limit_sz};
	dir_search_bfs(path, &dir_search_callbk, &args);

	return index;
}
