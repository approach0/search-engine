#pragma once
#include <stdlib.h>
#include "tree/tree.h"

#define MAX_TEX_TR_DEPTH   32
#define MAX_PARSER_ERR_STR 1024

struct subpath {
	uint32_t         node_id[MAX_TEX_TR_DEPTH]; 
	uint32_t         token_id[MAX_TEX_TR_DEPTH]; 
	uint32_t         fan[MAX_TEX_TR_DEPTH]; 
	uint32_t         symbol_id;
	struct list_node ln;
};

struct tex_parse_ret {
	uint32_t         code;	
	char             msg[MAX_PARSER_ERR_STR];
	list             subpaths;
};

struct tex_parse_ret tex_parse(const char *, size_t len);
