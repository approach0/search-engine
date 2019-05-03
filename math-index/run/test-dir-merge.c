#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mhook/mhook.h"
#include "math-index.h"
#include "subpath-set.h"

enum dir_merge_ret
on_dir_merge(
	char (*full_paths)[MAX_DIR_PATH_NAME_LEN],
	char (*base_paths)[MAX_DIR_PATH_NAME_LEN],
	struct subpath_ele **eles, uint32_t n_postings,
	uint32_t level, void *args)
{
	for (int i = 0; i < n_postings; i++) {
		printf("full: %s\n", full_paths[i]);
		printf("base: %s\n", base_paths[i]);
	}
	printf("\n");

	return DIR_MERGE_RET_CONTINUE;
}

int main(int argc, char *argv[])
{
	//const char tex[] = "\\qvar\\alpha+xy";
	//const char tex[] = "\\alpha + \\sqrt b +xy";
	const char tex[] = "a+xy";
	struct tex_parse_ret parse_ret;

	math_index_t index = math_index_open("./tmp", MATH_INDEX_READ_ONLY);

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	printf("search: `%s'\n", tex);
	parse_ret = tex_parse(tex, 0, false, false);

	if (parse_ret.code != PARSER_RETCODE_ERR) {
		subpaths_print(&parse_ret.subpaths, stdout);

		printf("calling math_index_dir_merge()...\n");

		int n;
		list subpath_set = dir_merge_subpath_set(
			DIR_PATHSET_PREFIX_PATH, &parse_ret.subpaths, &n
		);
		math_index_dir_merge(
			index, DIR_MERGE_BREADTH_FIRST,
			DIR_PATHSET_PREFIX_PATH, subpath_set, n,
			&on_dir_merge, NULL
		);

		subpath_set_free(&subpath_set);
		subpaths_release(&parse_ret.subpaths);
	} else {
		printf("parser error: %s\n", parse_ret.msg);
	}

	math_index_close(index);

	mhook_print_unfree();
	return 0;
}
