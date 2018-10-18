#include <stdlib.h>
#include <string.h>
#include "mhook/mhook.h"
#include "common/common.h"
#include "search.h"

const char index_path[] = "/home/tk/index-fix-decimal-and-use-latexml";
struct subpath_ele *out_eles[MAX_MERGE_POSTINGS];

static enum dir_merge_ret
test( /* add (l1) path posting lists into l2 posting list */
	char (*full_paths)[MAX_MERGE_DIRS], char (*base_paths)[MAX_MERGE_DIRS],
	struct subpath_ele **eles, uint32_t n_eles, uint32_t level, void *_args)
{
	for (uint32_t i = 0; i < n_eles; i++) {
		out_eles[i] = eles[i];
		printf("posting_list[%u]: %s\n", i, base_paths[i]);
	}
	return DIR_MERGE_RET_CONTINUE;
}

int main()
{
	struct indices indices;
	struct math_qry_struct mqs;
	struct math_pruner pruner;

	printf("opening index at path: `%s' ...\n", index_path);
	if (indices_open(&indices, index_path, INDICES_OPEN_RD)) {
		printf("index open failed.\n");
		goto close;
	}

	math_qry_prepare(&indices, "a+bc+defgh", &mqs);
	optr_print((struct optr_node*)mqs.optr, stdout);

	math_index_dir_merge(
		indices.mi, DIR_MERGE_DIRECT, DIR_PATHSET_PREFIX_PATH,
		mqs.subpath_set, mqs.n_uniq_paths, &test, NULL);
	
	memset(&pruner, 0, sizeof(struct math_pruner));
	math_pruner_init(&pruner, mqs.n_qry_nodes, out_eles, mqs.n_uniq_paths);
	
	math_pruner_print(&pruner);

	math_qry_free(&mqs);
	math_pruner_free(&pruner);

close:
	printf("closing index...\n");
	indices_close(&indices);

	mhook_print_unfree();
	return 0;
}
