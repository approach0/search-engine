#include <stddef.h>
#include <wchar.h>
#include "../config/config.h"
#include "../list/list.h"

struct term_list_node {
	wchar_t          term[MAX_TERM_STR_LEN];
	struct list_node ln;
};

int   text_segment_init();
list  text_segment(char *text);
void  text_segment_free();
