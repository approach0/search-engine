#include <stdio.h>
#include <stdlib.h>

#include "common/common.h"
#include "mhook/mhook.h"
#include "strmap/strmap.h"
#include "tex-parser/head.h" /* for trans_symbol */
#include "dir-util/dir-util.h"
#include "math-index/math-index.h"
#include "postlist-codec/postlist-codec.h"
#include "postlist/postlist.h"
#include "postlist/math-postlist.h"

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

		prinfo("[%u, %u, %u, %u]", disk_item->exp_id, disk_item->doc_id,
		                           disk_item->n_lr_paths, disk_item->n_paths);

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
			prinfo("[%u ~ %u, %s (%u)]", p->leaf_id, p->subr_id,
			       trans_symbol(p->lf_symb), p->lf_symb);
		}

		fl_sz = postlist_write(mem_po, &mem_item, sizeof(mem_item));
		if (fl_sz)
			printf("flush %lu bytes.\n", fl_sz);

	} while (math_posting_next(disk_po));

	fl_sz = postlist_write_complete(mem_po);
	printf("final flush %lu bytes.\n", fl_sz);

	return mem_po;
}

void
print_postlist(struct postlist *po)
{
	struct math_postlist_item *pi;
	postlist_start(po);
	PTR_CAST(codec, struct postlist_codec, po->buf_arg);

	do {
		pi = postlist_cur_item(po);
		postlist_print(pi, 1, codec->fields);

	} while (postlist_next(po));

	postlist_finish(po);
	printf("\n");
}

static enum ds_ret
dir_srch_callbk(const char* path, const char *srchpath,
                uint32_t level, void *arg)
{
	enum ds_ret ret = DS_RET_CONTINUE;
	math_posting_t *po;
	strmap_t math_dict = (strmap_t)arg;
	PTR_CAST(key, char, srchpath);

	po = math_posting_new_reader(NULL, path);

	if (!math_posting_start(po)) {
		/* this directory does not have index file */
		goto next;
	}

	printf("Caching %s\n", srchpath);
	math_dict[[key]] = fork_math_postlist(po);

next:
	math_posting_finish(po);
	math_posting_free_reader(po);

	return ret;
}

int main()
{
	strmap_t math_dict = strmap_new();

	dir_search_bfs("./tmp/prefix", &dir_srch_callbk, math_dict);

	if (math_dict["./ONE/ADD"]) {
		print_postlist(math_dict["./ONE/ADD"]);
		postlist_free(math_dict["./ONE/ADD"]);
	}
	if (math_dict["./VAR/ADD"]) {
		print_postlist(math_dict["./VAR/ADD"]);
		postlist_free(math_dict["./VAR/ADD"]);
	}
	if (math_dict["./VAR/TIMES"]) {
		print_postlist(math_dict["./VAR/TIMES"]);
		postlist_free(math_dict["./VAR/TIMES"]);
	}
	if (math_dict["./VAR/TIMES/ADD"]) {
		print_postlist(math_dict["./VAR/TIMES/ADD"]);
		postlist_free(math_dict["./VAR/TIMES/ADD"]);
	}

	strmap_free(math_dict);

	mhook_print_unfree();
	return 0;
}
