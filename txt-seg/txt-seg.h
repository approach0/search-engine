#pragma once
#include <wchar.h>
#include <stdbool.h>
#include "config.h"
#include "list/list.h"

#define MAX_TERM_WSTR_LEN 32

enum term_type {
	TERM_T_ENGLISH,
	TERM_T_CHINESE,
};

struct term_list_node {
	wchar_t          term[MAX_TERM_WSTR_LEN];
	uint32_t         begin_pos, end_pos;
	enum term_type   type;
	struct list_node ln;
};

int   text_segment_init(const char *dict_path);
list  text_segment(const char *text);
bool  text_segment_insert_usrterm(const char *term);
void  text_segment_free(void);
