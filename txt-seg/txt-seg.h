#include <stddef.h>
#include <wchar.h>
#include "config.h"
#include "list.h"

struct term_list_node {
	wchar_t          term[MAX_TERM_STR_LEN];
	struct list_node ln;
};

list text_segment(wchar_t *text, size_t len);
