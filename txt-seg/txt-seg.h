#ifndef TXT_SEG_H
#define TXT_SEG_H

#include <stddef.h>
#include <wchar.h>
#include "../include/config.h"
#include "list.h"

struct term_list_node {
	wchar_t          term[MAX_TERM_STR_LEN];
	struct list_node ln;
};

int   text_segment_init();
list  text_segment(char *text);
void  text_segment_free();

#endif
