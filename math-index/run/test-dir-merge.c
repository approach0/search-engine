#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math-index.h"
#include "subpath-set.h"

enum dir_merge_ret
on_dir_merge(math_posting_t postings[MAX_MATH_PATHS], uint32_t n_postings,
             uint32_t level, void *args)
{
	uint32_t i, j;
	math_posting_t po;
	struct subpath_ele *ele;
	struct subpath *sp;
	const char *fullpath;

	for (i = 0; i < n_postings; i++) {
		po = postings[i];
		ele = math_posting_get_ele(po);
		fullpath = math_posting_get_pathstr(po);

		printf("posting[%u]: %s ", i, fullpath);

		printf("(duplicates: ");
		for (j = 0; j <= ele->dup_cnt; j++) {
			sp = ele->dup[j];
			printf("path#%u ", sp->path_id);
		}
		printf(")\n");
	}
	printf("======\n");

	return DIR_MERGE_RET_CONTINUE;
}

int main(int argc, char *argv[])
{
	//const char tex[] = "\\qvar\\alpha+xy";
	//const char tex[] = "\\alpha + \\sqrt b +xy";
	const char tex[] = "\\alpha+xy";
	struct tex_parse_ret parse_ret;

	math_index_t index = math_index_open("./tmp", MATH_INDEX_READ_ONLY);

	if (index == NULL) {
		printf("cannot open index.\n");
		return 1;
	}

	printf("search: `%s'\n", tex);
	parse_ret = tex_parse(tex, 0, false);

	if (parse_ret.code != PARSER_RETCODE_ERR) {
		subpaths_print(&parse_ret.subpaths, stdout);

		printf("calling math_index_dir_merge()...\n");
		math_index_dir_merge(index, DIR_MERGE_DEPTH_FIRST, DIR_PATHSET_LEAFROOT_PATH,
		                     &parse_ret.subpaths, &on_dir_merge, NULL);
		
		subpaths_release(&parse_ret.subpaths);
	} else {
		printf("parser error: %s\n", parse_ret.msg);
	}

	math_index_close(index);
	return 0;
}
