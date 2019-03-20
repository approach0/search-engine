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
		/* overwrite output file ... */
		const char graph_output_file[] = "./graph.tmp";
		FILE *fh = fopen(graph_output_file, "w");
		fprintf(fh, "%%%% Render at %s\n",
		        "https://mermaidjs.github.io/mermaid-live-editor");

		/* Add to the history. */
		linenoiseHistoryAdd(line);

		ret = tex_parse(line, 0, true, false);

		if (ret.code != PARSER_RETCODE_ERR) {
			if (ret.operator_tree) {
				printf("Outputing graph in %s ...\n", graph_output_file);

				char *color[] = {"blue", "red", "green"};
				uint32_t node_map[1024] = {0};
				node_map[1] = 1;
				node_map[2] = 3;
				node_map[3] = 2;
				node_map[4] = 3;
				node_map[5] = 2;
				node_map[6] = 3;

				sds out = sdsempty();
				optr_graph_print(ret.operator_tree, color, node_map, 3, &out);
				fprintf(fh, "%s", out);
				sdsfree(out);

				optr_release((struct optr_node*)ret.operator_tree);
			}

			subpaths_release(&ret.subpaths);
		} else {
			printf("error message:%s\n", ret.msg);
		}

		fclose(fh);
		free(line);
	}

	printf("\n");

	mhook_print_unfree();
	return 0;
}
