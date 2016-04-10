#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "tex-parser.h"
#include "vt100-color.h"

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
		ret = tex_parse(line, 0);
		printf("return code:%s\n", (ret.code == PARSER_RETCODE_SUCC) ? "SUCC" : "ERR");
		printf("return message:%s\n", ret.msg);
		free(line);
	}

	printf("\n");
	return 0;
}
