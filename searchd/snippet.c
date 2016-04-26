#include <stdio.h>
#include <stdlib.h>

#include "list/list.h"
#include "snippet.h"

void snippet_rst_pos(list* pos_li)
{
}

void snippet_add_pos(list* pos_li, char* keystr,
                     uint32_t doc_pos, uint32_t n_bytes)
{
}

struct snippet
snippet_read(FILE* fh, list* pos_li)
{
	struct snippet ret;
	return ret;
}

struct snippet
snippet_read_blob(void* blob, size_t blob_sz, list* pos_li)
{
	struct snippet ret;
	return ret;
}

char *snippet_highlight(struct snippet* snippet, list* pos_li,
                        char *open, char *close)
{
	return malloc(1);
}
