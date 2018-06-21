#include <stdio.h>

#include "common/common.h"
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

static struct postlist *
fork_math_postlist(math_posting_t *disk_po)
{
	struct postlist *mem_po;
	struct math_posting_item_v2 *disk_item;
	struct math_pathinfo_v2      pathinfo[MAX_MATH_PATHS];
	struct math_postlist_item    mem_item;
	size_t fl_sz;

	mem_po = math_postlist_create_compressed();
	
	do {
		/* read posting list item */
		disk_item = (struct math_posting_item_v2*)math_posting_current(disk_po);

#ifdef DEBUG_MATH_POSTLIST_CACHE
		prinfo("[%u, %u, %u, %u]", disk_item->exp_id, disk_item->doc_id,
		                           disk_item->n_lr_paths, disk_item->n_paths);
#endif

		mem_item.doc_id     = disk_item->doc_id;
		mem_item.exp_id     = disk_item->exp_id;
		mem_item.n_lr_paths = disk_item->n_lr_paths;
		mem_item.n_paths    = disk_item->n_paths;

		/* then read pathinfo items */
		if (math_posting_pathinfo_v2(disk_po, disk_item->pathinfo_pos,
		                             disk_item->n_paths, pathinfo)) {
			prerr("cannot retrieve pathinfo_v2.");
			break;
		}

		/* copy pathinfo items */
		for (int i = 0; i < disk_item->n_paths; i++) {
			struct math_pathinfo_v2 *p = pathinfo + i;
			mem_item.subr_id[i] = p->subr_id;
			mem_item.leaf_id[i] = p->leaf_id;
			mem_item.lf_symb[i] = p->lf_symb;
#ifdef DEBUG_MATH_POSTLIST_CACHE
			prinfo("[%u ~ %u, %s (%u)]", p->leaf_id, p->subr_id,
			       trans_symbol(p->lf_symb), p->lf_symb);
#endif
		}

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

static enum ds_ret
dir_srch_callbk(const char* path, const char *srchpath,
                uint32_t level, void *arg)
{
	enum ds_ret ret = DS_RET_CONTINUE;
	struct postlist *mem_po;
	math_posting_t *disk_po;
	PTR_CAST(key, char, srchpath);

	PTR_CAST(cache, struct math_postlist_cache, arg);
	strmap_t path_dict = cache->path_dict;

	disk_po = math_posting_new_reader(NULL, path);

	if (!math_posting_start(disk_po)) {
		/* this directory does not have index file */
		goto next;
	}

#ifdef DEBUG_MATH_POSTLIST_CACHE
	printf("Caching %s\n", srchpath);
#endif
	mem_po = fork_math_postlist(disk_po);

	if (cache->postlist_sz + mem_po->tot_sz > cache->limit_sz) {
		prinfo("math postlist cache reaches size limit.");
		postlist_free(mem_po);
		ret = DS_RET_STOP_ALLDIR;
	} else {
		path_dict[[key]] = mem_po;
		cache->postlist_sz += mem_po->tot_sz;
	}

next:
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
		char *key = iter.cur->keystr;
		postlist_free(cache.path_dict[[key]]);
	}

	strmap_free(cache.path_dict);
}

int
math_postlist_cache_add(struct math_postlist_cache *cache, const char *dir)
{
	size_t postlist_sz = cache->postlist_sz;
	dir_search_bfs(dir, &dir_srch_callbk, cache);

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
		char *key = iter.cur->keystr;
		if (print) printf("cached math path: %s\n", key);
		cnt ++;
	}
	return cnt;
}
