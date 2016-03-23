#pragma once
#include <wchar.h>
#include "config.h"
#include "list/list.h"

struct term_list_node {
	wchar_t          term[MAX_TERM_WSTR_LEN];
	struct list_node ln;
};

int   text_segment_init(const char *dict_path);
list  text_segment(const char *text);
void  text_segment_free();
