#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "tex-parser.h"
#include "vt100-color.h"

struct optr_node;
void optr_print(struct optr_node*, FILE*);
void optr_release(struct optr_node*);

int main()
{
	char *line;
	struct tex_parse_ret ret;

	/* disable readline auto-complete */
	rl_bind_key('\t', rl_abort);

	while (1) {
		line = readline(C_CYAN "edit: " C_RST);

		if (NULL == line)
			break;

		add_history(line);

		ret = tex_parse(line, 0, true);

		printf("return code:%s\n", (ret.code == PARSER_RETCODE_SUCC) ? "SUCC" : "ERR");

		if (ret.code == PARSER_RETCODE_SUCC) {
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
	return 0;
}
