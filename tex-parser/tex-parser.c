#include <stdlib.h>
#include <string.h>
#include "tex-parser.h"
#include "yy.h"

static char *mk_scan_buf(const char *str, size_t *out_sz)
{
	char *buf;
	*out_sz = strlen(str) + 3;
	buf = malloc(*out_sz);
	sprintf(buf, "%s\n_", str);
	buf[*out_sz - 2] = '\0';

	return buf;
}

struct tex_parse_ret
tex_parse(const char *tex_str, size_t len)
{
	struct tex_parse_ret ret; 
	YY_BUFFER_STATE state_buf;
	char *scan_buf;
	size_t scan_buf_sz;

	scan_buf = mk_scan_buf(tex_str, &scan_buf_sz);
	state_buf = yy_scan_buffer(scan_buf, scan_buf_sz);

	yyparse();

	yy_delete_buffer(state_buf);
	free(scan_buf);

	ret.code = 0;
	return ret;
}
