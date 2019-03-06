#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "math-index.h"

static uint32_t doc_id = 1;

static void reset()
{
	doc_id = 1;
}

static uint32_t gen_doc_id()
{
	int sel = random() % 4;
	switch (sel) {
	case 0: /* large increase */
		doc_id += random() % 999;
		break;
	case 1: /* small increase */
		doc_id += random() % 10;
		break;
	default: /* unchange */
		break;
	}
	return doc_id;
}

static uint32_t gen_exp_id(uint32_t doc)
{
	static uint32_t prev_doc_id = 0;
	static uint32_t exp_id = 1;

	if (prev_doc_id != doc)
		exp_id = 1;
	else
		exp_id += random() % 100;

	prev_doc_id = doc;
	return exp_id;
}

static uint32_t gen_path_num()
{
	int sel = random() % 5;
	switch (sel) {
	case 0: /* large number of paths */
		return (random() % 64) + 1;
	case 1: /* median number of paths */
		return (random() % 35) + 1;
	default: /* small number of paths */
		return (random() % 10) + 1;
	}
}

static void append_mock_item(const char *path, int gener)
{
	struct math_posting_item_v2 item;

	item.doc_id       = gen_doc_id();
	item.exp_id       = gen_exp_id(item.doc_id);
	item.n_lr_paths   = gen_path_num();
	item.n_paths      = gen_path_num();
	item.pathinfo_pos = get_pathinfo_len(path); /* to be appended */

	printf("doc#%u, exp#%u, lr#%u, %u paths:\n", item.doc_id, item.exp_id,
		item.n_lr_paths, item.n_paths);

	write_posting_item_v2(path, &item);

	/* now, mock pathinfo for this item */
	char fileinfo_path[2048];
	sprintf(fileinfo_path, "%s/" PATH_INFO_FNAME, path);
	FILE *fh = fopen(fileinfo_path, "a");
	if (fh == NULL)
		abort();

	for (int i = 0; i < item.n_paths; i++) {
		if (gener) {
			struct math_pathinfo_gener_v2 pathinfo;
			pathinfo.wild_id = (random() % 64) + 1;
			pathinfo.subr_id = (random() % 300) + 1;
			pathinfo.tr_hash = random() % 0xffff;
			pathinfo.op_hash = random() % 0xffff;
			uint64_t r1 = random(), r2 = random();
			pathinfo.wild_leaves = (r1 << 32) | r2;
			fwrite(&pathinfo, 1, sizeof(pathinfo), fh);

			printf("\t gener pathinfo %u, %u, 0x%x, 0x%x, 0x%lx \n",
				pathinfo.wild_id,
				pathinfo.subr_id,
				pathinfo.tr_hash,
				pathinfo.op_hash,
				pathinfo.wild_leaves
			);

		} else {
			struct math_pathinfo_v2 pathinfo;
			pathinfo.leaf_id = (random() % 64) + 1;
			pathinfo.subr_id = (random() % 300) + 1;
			pathinfo.lf_symb = random() % 0xffff;
			pathinfo.op_hash = random() % 0xffff;
			fwrite(&pathinfo, 1, sizeof(pathinfo), fh);

			printf("\t normal pathinfo %u, %u, %u, 0x%x \n",
				pathinfo.leaf_id,
				pathinfo.subr_id,
				pathinfo.lf_symb,
				pathinfo.op_hash
			);
		}
	}

	fclose(fh);
}

int main()
{
	const char gener_path[] = "./tmp/gener/ADD";
	const char prefix_path[] = "./tmp/prefix/VAR";

	srand(time(NULL));

	mkdir_p(gener_path);
	mkdir_p(prefix_path);

	for (int i = 0; i < 99; i++)
		append_mock_item(gener_path, 1);

	printf("\n");
	reset();

	for (int i = 0; i < 100; i++)
		append_mock_item(prefix_path, 0);
}
