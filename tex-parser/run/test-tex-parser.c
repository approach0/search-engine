#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "../include/vt100-color.h"
#include "tex-parser.h"

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
		free(line);
	}

	printf("\n");
	return 0;
}
