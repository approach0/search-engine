#include <stdio.h>
#include <stdlib.h>

#include "mhook/mhook.h"

#include "head.h"
#include "completion.h"

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

		ret = tex_parse(line);

		printf("return string: %s\n", ret.msg);
		printf("return code: %d\n", ret.code);

		if (ret.code != PARSER_RETCODE_ERR) {
			printf("Operator tree:\n");
			if (ret.operator_tree) {
				optr_print((struct optr_node*)ret.operator_tree, stdout);
				optr_release((struct optr_node*)ret.operator_tree);
			}
			printf("\n");

			printf("%u leaf-root paths\n", ret.lr_paths.n);
			lr_paths_print(&ret.lr_paths, stdout);
			lr_paths_release(&ret.lr_paths);
		} else {
			printf("error message:%s\n", ret.msg);
		}

		free(line);
	}

	printf("\n");

	mhook_print_unfree();
	return 0;
}
