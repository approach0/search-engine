#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"

#include "tex-parser.h"
#include "vt100-color.h"
#include "completion.h"

struct optr_node;
void optr_print(struct optr_node*, FILE*);
void optr_release(struct optr_node*);

int main()
{
	char *line;
	struct tex_parse_ret ret;

	/* hook up completion callback */
	linenoiseSetCompletionCallback(completion_callbk);

	/* FIXME: linenoise has memory leakages. However, because
	 * test-tex-parser does not care about leakage too much,
	 * we may leave this issue along. */

	while ((line = linenoise("edit: ")) != NULL) {
		/* Add to the history. */
		linenoiseHistoryAdd(line);

		ret = tex_parse(line, 0, true);

		printf("return code:%d\n", ret.code);

		if (ret.code != PARSER_RETCODE_ERR) {
			printf("Operator tree:\n");
			if (ret.operator_tree) {
				optr_print((struct optr_node*)ret.operator_tree, stdout);
				optr_release((struct optr_node*)ret.operator_tree);
			}
			printf("\n");

			printf("Subpaths (leaf-root paths/total subpaths = %u/%u):\n", 
				   ret.subpaths.n_lr_paths, ret.subpaths.n_subpaths);
			subpaths_print(&ret.subpaths, stdout);
			subpaths_release(&ret.subpaths);
		} else {
			printf("error message:%s\n", ret.msg);
		}

		free(line);
	}

	printf("\n");

	mhook_print_unfree();
	return 0;
}
