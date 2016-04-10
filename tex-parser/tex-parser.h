#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "list/list.h"

typedef uint16_t symbol_id_t;

#define PARSER_RETCODE_ERR  1
#define PARSER_RETCODE_SUCC 0
#define MAX_PARSER_ERR_STR  1024
#define MAX_SUBPATH_ID      64

enum subpath_type {
	SUBPATH_TYPE_GENERNODE,
	SUBPATH_TYPE_WILDCARD,
	SUBPATH_TYPE_NORMAL
};

struct subpath_node {
	uint32_t         token_id;
	uint32_t         sons;
	struct list_node ln;
};

struct subpath {
	uint32_t              path_id;
	list                  path_nodes;
	enum subpath_type     type;
	union {
		symbol_id_t       lf_symbol_id;
		symbol_id_t       ge_hash;
	};
	symbol_id_t           fr_hash;
	struct list_node      ln;
};

struct subpaths {
	list      li;
	uint32_t  n_lr_paths;
};

struct tex_parse_ret {
	uint32_t         code;
	char             msg[MAX_PARSER_ERR_STR];
	struct subpaths  subpaths;
};

struct tex_parse_ret tex_parse(const char *, size_t len);
