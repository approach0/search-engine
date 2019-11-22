#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for dup() */
#include <assert.h>

#include "common/common.h"
#include "linkli/list.h"
#include "dir-util/dir-util.h"
#include "tex-parser/head.h"

#include "config.h"
#include "math-index.h"
#include "subpath-set.h"

typedef uint32_t doc_id_t;

struct mk_path_str_arg {
	char **dest;
	bool skip_first;
	int max;
	int cnt;
};

struct codec_buf_struct_info *math_codec_info()
{
	struct codec_buf_struct_info *info;
	info = codec_buf_struct_info_alloc(
		N_MATH_INVLIST_ITEM_FIELDS,
		sizeof(struct math_invlist_item)
	);

#define SET_FIELD_INFO(_idx, _name, _codec) \
	info->field_info[_idx] = FIELD_INFO(struct math_invlist_item, _name, _codec); \
	strcpy(info->field_info[_idx].name, # _name)

	SET_FIELD_INFO(FI_DOCID, docID, CODEC_FOR_DELTA);
	SET_FIELD_INFO(FI_EXPID, expID, CODEC_FOR16);
	SET_FIELD_INFO(FI_SECT_ROOT, sect_root, CODEC_FOR16);
	SET_FIELD_INFO(FI_SECT_WIDTH, sect_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_ORIG_WIDTH, orig_width, CODEC_FOR8);
	SET_FIELD_INFO(FI_OFFSET, symbinfo_offset, CODEC_FOR_DELTA);

	return info;
}

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
static uint64_t
math_bufkey_64(struct invlist_iterator* iter, uint32_t idx)
{
	/* reversed key fields to cast to little endian 64-bit key */
	struct {
		struct {
			uint16_t sect_root;
			uint16_t expID;
		};
		uint32_t docID;
	} key;

	codec_buf_struct_info_t *c_info = iter->c_info;

	CODEC_BUF_GET(key.docID, iter->buf, FI_DOCID, idx, c_info);
	CODEC_BUF_GET(key.expID, iter->buf, FI_EXPID, idx, c_info);
	CODEC_BUF_GET(key.sect_root, iter->buf, FI_SECT_ROOT, idx, c_info);

	return *(uint64_t*)&key;
}
#else
#error("big-indian CPU is not supported yet.")
#endif

math_index_t
math_index_open(const char *path, const char *mode)
{
	math_index_t index;

	index = malloc(sizeof(struct math_index));

	sprintf(index->dir, "%s", path);
	sprintf(index->mode, "%s", mode);

	index->dict = strmap_new();
	index->cinfo = math_codec_info();

	{  /* read the stats value */
		char path[MAX_PATH_LEN];
		sprintf(path, "%s/%s.bin", index->dir, MSTATS_FNAME);
		FILE *fh_stats = fopen(path, "r");
		if (NULL == fh_stats) {
			memset(&index->stats, 0, sizeof index->stats);
		} else {
			fread(&index->stats, 1, sizeof index->stats, fh_stats);
			fclose(fh_stats);
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
	if (index->mode[0] != 'w')
		return;

	/* write the current stats values */
	char path[MAX_PATH_LEN];
	sprintf(path, "%s/%s.bin", index->dir, MSTATS_FNAME);
	FILE *fh_stats = fopen(path, "w");
	if (fh_stats) {
		fwrite(&index->stats, 1, sizeof index->stats, fh_stats);
		fclose(fh_stats);
	}
}

static void free_invlist_entry(struct math_invlist_entry *entry)
{
	if (entry->invlist)
		invlist_free(entry->invlist);

	if (entry->symbinfo_path)
		free(entry->symbinfo_path);

	if (entry->pf_path)
		free(entry->pf_path);

	free(entry);
}

void math_index_close(math_index_t index)
{
	math_index_flush(index);

	foreach (iter, strmap, index->dict) {
		P_CAST(entry, struct math_invlist_entry, iter->cur->value);
		free_invlist_entry(entry);
	}

	/* free member structure */
	strmap_free(index->dict);
	codec_buf_struct_info_free(index->cinfo);
	free(index);
}

void math_index_print(math_index_t index)
{
	printf("[math index] %s (memo_usage=%luKB, n_dict_ent=%u, "
		"n_tex=%u, N=%u, mode: %s)\n",
		index->dir, index->memo_usage / 1024, index->dict->length,
		index->stats.n_tex, index->stats.N, index->mode);

	int max = 100, cnt = 0;
	foreach (iter, strmap, index->dict) {
		P_CAST(entry, struct math_invlist_entry, iter->cur->value);

		if (entry->invlist->type == INVLIST_TYPE_ONDISK) {
			printf("[on-disk] %s ", iter->cur->keystr);
		} else {
			printf("[in-memo] %s ", iter->cur->keystr);
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

static int init_invlist_entry(struct math_invlist_entry *entry,
	struct codec_buf_struct_info *cinfo, const char *path)
{
	/* make path names */
	char invlist_path[MAX_PATH_LEN];
	char symbinfo_path[MAX_PATH_LEN];
	char pf_path[MAX_PATH_LEN];
	sprintf(invlist_path, "%s/%s", path, MINVLIST_FNAME);
	sprintf(symbinfo_path, "%s/%s.bin", path, SYMBINFO_FNAME);
	sprintf(pf_path, "%s/%s.bin", path, PATHFREQ_FNAME);

	/* create path strings */
	entry->symbinfo_path = strdup(symbinfo_path);
	entry->pf_path = strdup(pf_path);

	/* open inverted list on disk */
	entry->invlist = invlist_open(invlist_path, MATH_INDEX_BLK_LEN, cinfo);
	/* change default bufkey map */
	entry->invlist->bufkey = &math_bufkey_64;

	/* set path frequency */
	{ /* first read out value if there exists a file */
		FILE *fh = fopen(pf_path, "r");
		if (NULL != fh) {
			fread(&entry->pf, 1, sizeof entry->pf, fh);
			fclose(fh);
		} else {
			entry->pf = 0; /* first-time touch base */
		}
	}

	return 0;
}

static size_t
dict_entry_size(struct codec_buf_struct_info *cinfo, size_t str_len)
{
	size_t tot_sz = 0;

	tot_sz += sizeof(struct strmap_entry) + str_len;
	tot_sz += sizeof(datrie_state_t) * 2;

	tot_sz += sizeof(struct math_invlist_entry);
	tot_sz += sizeof(struct invlist);
	tot_sz += str_len * 2; /* estimated memory for two path strings */

	return tot_sz;
}

static size_t append_invlist(math_index_t index, char *path,
	struct subpath_ele *ele, doc_id_t docID, exp_id_t expID, uint32_t width)
{
	size_t flush_payload = 0;
	struct math_invlist_entry *entry = NULL;

	/* create a temporary entry */
	entry = malloc(sizeof *entry);
	init_invlist_entry(entry, index->cinfo, path);
	invlist_iter_t writer = invlist_writer(entry->invlist);

	/* open files */
	FILE *fh_symbinfo = fopen(entry->symbinfo_path, "a");
	FILE *fh_pf = fopen(entry->pf_path, "w");

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
		item.symbinfo_offset = ftell(fh_symbinfo);

#ifdef MATH_INDEX_SECTTR_PRINT
		printf("writing sector tree %u/%u: ",
			secttr.rootID, secttr.width);
#endif

		/* write sector tree structure */
		flush_payload = invlist_writer_write(writer, &item);

		/* prepare symbinfo structure */
		symbinfo.ophash = secttr.ophash;
		symbinfo.n_splits = MIN(ele->n_splits[i], MAX_INDEX_EXP_SYMBOL_SPLITS);
		for (int j = 0; j < symbinfo.n_splits; j++) {
			symbinfo.split[j].symbol = ele->symbol[i][j];
			symbinfo.split[j].splt_w = ele->splt_w[i][j];
#ifdef MATH_INDEX_SECTTR_PRINT
			printf("%s/%u ", trans_symbol(symbinfo.symbol[j]),
			                 symbinfo.splt_w[j]);
#endif
		}
#ifdef MATH_INDEX_SECTTR_PRINT
		printf("\n");
#endif

		/* write symbinfo structure, update offset by written bytes */
		flush_payload += fwrite(&symbinfo, 1, SYMBINFO_SIZE(symbinfo.n_splits),
		                        fh_symbinfo);

		/* update frequency statistics */
		entry->pf ++;
		rewind(fh_pf);
		fwrite(&entry->pf, 1, sizeof entry->pf, fh_pf);

		index->stats.N ++;
	}

	/* free temporary entry */
	free_invlist_entry(entry);
	invlist_iter_free(writer);

	/* close files to save OS file descriptor space */
	fclose(fh_symbinfo);
	fclose(fh_pf);

	return flush_payload;
}

static size_t
add_subpath_set(math_index_t index, linkli_t set,
                doc_id_t docID, exp_id_t expID, uint32_t width)
{
	const char *root_path = index->dir;
	size_t flush_sz = 0;
	foreach (iter, li, set) {
		char path[MAX_DIR_PATH_NAME_LEN] = "";
		char *p = path;
		p += sprintf(p, "%s/", root_path);

		struct subpath_ele *ele = li_entry(ele, iter->cur, ln);
		struct subpath *sp = ele->dup[0];

		/* TODO: wildcards path not supported yet */
		if (sp->type == SUBPATH_TYPE_GENERNODE)
			continue;

		if (0 != mk_path_str(sp, ele->prefix_len, p)) {
			/* specified prefix_len is greater than actual path length */
			continue;
		}

		/* make directory */
#ifdef DEBUG_SUBPATH_SET
		printf("mkdir -p %s\n", path);
#endif
		mkdir_p(path);

		flush_sz += append_invlist(index, path, ele, docID, expID, width);
	}

	return flush_sz;
}

size_t math_index_add(math_index_t index,
	doc_id_t docID, exp_id_t expID, struct subpaths subpaths)
{
	if (index->mode[0] != 'w') {
		prerr("math_index_add needs writing permission.\n");
		return 0;
	}

	if (subpaths.n_lr_paths > MAX_MATH_PATHS) {
		prerr("too many subpaths (%lu > %lu), abort.\n",
			subpaths.n_lr_paths, MAX_MATH_PATHS);
		return 0;
	}

	linkli_t set = subpath_set(subpaths, SUBPATH_SET_DOC);

	/* for indexing, we do not accept wildcards */
	remove_wildcards(&set);

#ifdef DEBUG_SUBPATH_SET
	printf("subpath set (size=%d)\n", li_size(set));
	print_subpath_set(set);
#endif

	size_t flush_sz =
	add_subpath_set(index, set, docID, expID, subpaths.n_lr_paths);

	index->stats.n_tex ++;

	li_free(set, struct subpath_ele, ln, free(e));
	return flush_sz;
}

static struct invlist *fork_invlist(struct invlist *disk)
{
	codec_buf_struct_info_t *c_info = disk->c_info;

	struct invlist *memo = invlist_open(NULL, MATH_INDEX_BLK_LEN, c_info);

	/* change default bufkey map */
	memo->bufkey = &math_bufkey_64;

	/* get writer */
	invlist_iter_t  memo_writer = invlist_writer(memo);

	/* write all items to memory */
	foreach (iter, invlist, disk) {
		/* read item from disk */
		struct math_invlist_item item;
		invlist_iter_read(iter, &item);

		/* write item to memory */
		invlist_writer_write(memo_writer, &item);
	}

	invlist_writer_flush(memo_writer);

	/* free writer and disk reader */
	invlist_iter_free(memo_writer);
	invlist_free(disk);

	return memo;
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
	sprintf(test_path, "%s/%s.bin", path, SYMBINFO_FNAME);
	if (!file_exists(test_path) /* no inverted list here */)
		return DS_RET_CONTINUE;

	/* unexpected duplication check */
	if (NULL != strmap_lookup(dict, key_path)) {
		prerr("key `%s' already exists when loading math index.", key_path);
		return DS_RET_CONTINUE;
	}

	/* memory usage predict */
	size_t cost = dict_entry_size(index->cinfo, strlen(key_path));
	if (index->memo_usage + cost > args->limit_sz) {
		prinfo("math index cache size reaches limit, stop caching.");
		return DS_RET_STOP_ALLDIR;
	}
	index->memo_usage += cost;

	/* allocate entry */
	struct math_invlist_entry *entry;
	entry = malloc(sizeof *entry);
	dict[[key_path]] = entry;

	/* open invlist and entry metadata */
	init_invlist_entry(entry, index->cinfo, path);

	/* uncomment to test on-disk inverted list */
	//invlist_print_as_decoded_ints(entry->invlist);

	/* fork on-disk inverted list into memory */
	entry->invlist = fork_invlist(entry->invlist);
	index->memo_usage += entry->invlist->tot_payload_sz;

	printf(ES_RESET_LINE);
	printf("[caching @ level %u, memory usage: %.2f %%] %s ", level,
		100.f * index->memo_usage / args->limit_sz, key_path);
	fflush(stdout);

	return DS_RET_CONTINUE;
}

int math_index_load(math_index_t index, size_t limit_sz)
{
	/* load index from on-disk directories by BFS */
	struct math_index_load_arg args = {index, limit_sz};
	dir_search_bfs(index->dir, &dir_search_callbk, &args);
	printf("\n");

	return 0;
}

struct math_invlist_entry_reader
math_index_lookup(math_index_t index, const char *key_)
{
	struct math_invlist_entry_reader entry_reader = {0};
	struct math_invlist_entry *entry_ptr, entry = {0};
	P_CAST(key_path, char, key_);

	/* lookup from cache first */
	if ((entry_ptr = strmap_lookup(index->dict, key_path))) {
		entry_reader.reader = invlist_iterator(entry_ptr->invlist);
		entry_reader.pf = entry_ptr->pf;

		entry_reader.fh_symbinfo = fopen(entry_ptr->symbinfo_path, "r");

		entry_reader.medium = MATH_READER_MEDIUM_INMEMO;
		return entry_reader;
	}

	/* if not there, look up from disk directory */
	char path[MAX_PATH_LEN];
	sprintf(path, "%s/%s/%s.bin", index->dir, key_, SYMBINFO_FNAME);

	if (file_exists(path)) {
		sprintf(path, "%s/%s", index->dir, key_);
		init_invlist_entry(&entry, index->cinfo, path);

		entry_reader.reader = invlist_iterator(entry.invlist);
		/* it is safe to free inverted list here since on-disk iterator
		 * has only copied its path string and for reading, the invlist
		 * pointer will not be used inside of invlist iterator. */
		invlist_free(entry.invlist);

		entry_reader.pf = entry.pf;
		entry_reader.fh_symbinfo = fopen(entry.symbinfo_path, "r");

		entry_reader.medium = MATH_READER_MEDIUM_ONDISK;

		if (entry.symbinfo_path) free(entry.symbinfo_path);
		if (entry.pf_path) free(entry.pf_path);
		return entry_reader;
	}

	return entry_reader;
}
