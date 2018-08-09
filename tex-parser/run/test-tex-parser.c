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

		ret = tex_parse(line, 0, true);

		printf("return string: %s\n", ret.msg);
		printf("return code: %d\n", ret.code);

		if (ret.code != PARSER_RETCODE_ERR) {
			printf("Operator tree:\n");
			if (ret.operator_tree) {
				optr_print((struct optr_node*)ret.operator_tree, stdout);

				/* output graph file */
				{
					const char graph_output_file[] = "./graph.tmp";
					FILE *fh = fopen(graph_output_file, "w");
					printf("Outputing graph in mermaid.js ...\n");
					fprintf(fh, "%%%% Render at %s\n",
					        "https://mermaidjs.github.io/mermaid-live-editor");
					char *color[1024] = {0};
					color[1] = "blue";
					color[3] = "red";
					optr_graph_print(ret.operator_tree, color, fh);
					fclose(fh);
				}

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
