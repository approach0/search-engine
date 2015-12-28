#ifndef TEX_PARSER_H
#define TEX_PARSER_H

#include <stdlib.h>
#include "../list/list.h"
#include "../include/config.h"

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
};

struct tex_parse_ret tex_parse(const char *, size_t len);

void           *op_tr_print(void *tr);
struct subpath *op_tr_subpaths(void *);
void            op_tr_subpaths_print(struct subpath *);

#endif
