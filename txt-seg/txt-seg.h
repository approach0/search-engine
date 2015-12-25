#include <stddef.h>
#include "config.h"
#include "list.h"

struct term_list_node {
	char             term[MAX_TERM_STR_LEN];
	struct list_node ln;
};

int   text_segment_init();
list  text_segment(char *text, size_t arr_len);
void  text_segment_free();
