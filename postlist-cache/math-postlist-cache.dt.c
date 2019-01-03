#include <stdio.h>

#include "common/common.h"
#include "sds/sds.h"
#include "dir-util/dir-util.h"
#include "math-index/math-index.h"
#include "postlist-codec/postlist-codec.h"
#include "postlist/postlist.h"
#include "postlist/math-postlist.h"

#include "config.h"
#include "math-postlist-cache.h"

#ifdef DEBUG_MATH_POSTLIST_CACHE
char *trans_symbol(int);
#endif

void disk_item_v2_to_mem_item(
	struct math_postlist_item *mem_item,
	struct math_posting_compound_item_v2 *disk_item)
{
#ifdef DEBUG_MATH_POSTLIST_CACHE
	prinfo("[%u, %u, %u, %u]", disk_item->exp_id, disk_item->doc_id,
	                           disk_item->n_lr_paths, disk_item->n_paths);
#endif
	mem_item->doc_id     = disk_item->doc_id;
	mem_item->exp_id     = disk_item->exp_id;
	mem_item->n_lr_paths = disk_item->n_lr_paths;
	mem_item->n_paths    = disk_item->n_paths;

	/* copy pathinfo items */
	for (int i = 0; i < disk_item->n_paths; i++) {
		struct math_pathinfo_v2 *p = disk_item->pathinfo + i;
		mem_item->subr_id[i] = p->subr_id;
		mem_item->leaf_id[i] = p->leaf_id;
		mem_item->lf_symb[i] = p->lf_symb;
#ifdef DEBUG_MATH_POSTLIST_CACHE
		prinfo("[%u ~ %u, %s (%u)]", p->leaf_id, p->subr_id,
		       trans_symbol(p->lf_symb), p->lf_symb);
#endif
	}
}

struct postlist *
fork_math_postlist(math_posting_t *disk_po)
{
	struct postlist *mem_po;
	struct math_postlist_item mem_item;
	size_t fl_sz;

	mem_po = math_postlist_create_compressed();

	do {
		PTR_CAST(disk_item, struct math_posting_compound_item_v2,
		         math_posting_cur_item_v2(disk_po));
		disk_item_v2_to_mem_item(&mem_item, disk_item);

		fl_sz = postlist_write(mem_po, &mem_item, sizeof(mem_item));
#ifdef DEBUG_MATH_POSTLIST_CACHE
		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);
#endif
	} while (math_posting_next(disk_po));

	fl_sz = postlist_write_complete(mem_po);
#ifdef DEBUG_MATH_POSTLIST_CACHE
	printf("final flush %lu bytes.\n", fl_sz);
#else
	(void)fl_sz;
#endif

	return mem_po;
}

struct math_postlist_cache_arg {
	struct math_postlist_cache *cache;
	char *prefix_dir;
	uint64_t tot_cnt;
};

static enum ds_ret
dir_srch_callbk(const char* path, const char *srchpath,
                uint32_t level, void *arg)
{
	enum ds_ret ret = DS_RET_CONTINUE;
	struct postlist *mem_po;
	math_posting_t *disk_po;

	if (level > MAX_MATH_CACHE_DIR_LEVEL)
		return ret;

	PTR_CAST(mpca, struct math_postlist_cache_arg, arg);
	sds key = sdsnew(mpca->prefix_dir);
	struct math_postlist_cache *cache = mpca->cache;
	strmap_t path_dict = cache->path_dict;

	disk_po = math_posting_new_reader(path);

	if (!math_posting_start(disk_po)) {
		/* this directory does not have index file */
		goto next;
	}

	mem_po = fork_math_postlist(disk_po);
	mpca->tot_cnt ++;

	/* print progress */
	printf(ES_RESET_LINE);
	printf("[Cached %8lu lists, %.3f/%.3f MB] %s",
		mpca->tot_cnt, (float)cache->postlist_sz / __1MB__,
		(float)cache->limit_sz / __1MB__, srchpath);
	fflush(stdout);

//	if (0 == strcmp(srchpath, "./ZERO/ARROW")) {
//		print_postlist(mem_po);
//	}

	if (cache->postlist_sz + mem_po->tot_sz > cache->limit_sz) {
		// prinfo("math postlist cache reaches size limit.");
		postlist_free(mem_po);
		ret = DS_RET_STOP_ALLDIR;
	} else {
		key = sdscat(key, srchpath + 1);
		path_dict[[key]] = mem_po;
		cache->postlist_sz += mem_po->tot_sz;
	}

next:
	sdsfree(key);
	math_posting_finish(disk_po);
	math_posting_free_reader(disk_po);

	return ret;
}


struct math_postlist_cache
math_postlist_cache_new()
{
	struct math_postlist_cache cache = {NULL, 0, 0};
	cache.path_dict = strmap_new();
	cache.limit_sz = DEFAULT_MATH_CACHE_SZ;
	return cache;
}

void
math_postlist_cache_free(struct math_postlist_cache cache)
{
	foreach (iter, strmap, cache.path_dict) {
		char *key = iter->cur->keystr;
		struct postlist *po = cache.path_dict[[key]];
		if (po) postlist_free(po);
	}

	strmap_free(cache.path_dict);
}

int
math_postlist_cache_add(struct math_postlist_cache *cache, const char *dir)
{
	size_t postlist_sz = cache->postlist_sz;
	struct math_postlist_cache_arg args = {
		cache, (char *)"./" PREFIX_PATH_NAME, 0
	};
	dir_search_bfs(dir, &dir_srch_callbk, &args);
	// dir_search_podfs(dir, &dir_srch_callbk, &args);
	printf("\n");

	return (postlist_sz == cache->postlist_sz);
}

void*
math_postlist_cache_find(struct math_postlist_cache cache, char* path)
{
	strmap_t d = cache.path_dict;
	return d[[path]];
}

size_t
math_postlist_cache_list(struct math_postlist_cache c, int print)
{
	size_t cnt = 0;
	foreach (iter, strmap, c.path_dict) {
		char *key = iter->cur->keystr;
		if (print) printf("cached math path: %s\n", key);
		cnt ++;
	}
	return cnt;
}

int
math_postlist_cache_add_list(struct math_postlist_cache *cache, const char *dir)
{
	FILE *fh = NULL;
	char _line[4096];
	fh = fopen("./cache-list.tmp", "r");
	if (fh == NULL)
		return 1;
	
	while (fgets(_line, 4096, fh) != NULL) {
		/* line string */
		sds line = sdsnew(_line);
		sdstrim(line, "\n");
		/* absolute path string */
		sds abs_path = sdsnew(dir);
		abs_path = sdscat(abs_path, "/");
		abs_path = sdscat(abs_path, line);

		printf(ES_RESET_LINE);
		printf("[from cache-list] %s", line);
		fflush(stdout);

		math_posting_t *disk_po = math_posting_new_reader(abs_path);

		if (!math_posting_start(disk_po)) {
			/* this directory does not have index file */
			printf("Cannot find posting list @ %s\n", abs_path);
			goto next;
		}

		struct postlist *mem_po = fork_math_postlist(disk_po);

		strmap_t path_dict = cache->path_dict;
		if (NULL == path_dict[[line]]) {
			path_dict[[line]] = mem_po;
			cache->postlist_sz += mem_po->tot_sz;
		} else {
			printf("[duplicate cache] %s\n", line);
			postlist_free(mem_po);
		}

		math_posting_finish(disk_po);
next:
		sdsfree(line);
		sdsfree(abs_path);
		math_posting_free_reader(disk_po);
	}

	printf("\n");
	fclose(fh);
	return 0;
}
