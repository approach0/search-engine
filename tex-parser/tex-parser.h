#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "list/list.h"

typedef uint16_t symbol_id_t;
typedef uint16_t fingerpri_t;

#define PARSER_RETCODE_WARN 2
#define PARSER_RETCODE_ERR  1
#define PARSER_RETCODE_SUCC 0

#define MAX_PARSER_ERR_STR  1024

enum subpath_type {
	SUBPATH_TYPE_GENERNODE,
	SUBPATH_TYPE_WILDCARD,
	SUBPATH_TYPE_NORMAL
};

struct subpath_node {
	uint32_t         symbol_id;
	uint32_t         token_id;
	uint32_t         node_id;
	uint32_t         sons;
	struct list_node ln;
};

struct subpath {
	uint32_t              path_id; /* assigned pathID, can be re-assigned, e.g. prepare_math_qry() */
	uint32_t              leaf_id; /* original nodeID for the path leaf */
	uint32_t              n_nodes;
	list                  path_nodes;
	enum subpath_type     type;
	bool                  pseudo;
	uint64_t              conjugacy;
	union {
		symbol_id_t       lf_symbol_id;
		symbol_id_t       subtree_hash;
	};
	fingerpri_t           fingerprint; /* operator fingerprint, each operator 4 bits */
	struct list_node      ln;
};

struct subpaths {
	list      li;
	uint32_t  n_lr_paths; /* number of leaf-root paths */
	uint32_t  n_subpaths; /* total subpaths generated. */
};

struct tex_parse_ret {
	uint32_t         code;
	char             msg[MAX_PARSER_ERR_STR];
	struct subpaths  subpaths;
	void            *operator_tree;
};

struct tex_parse_ret tex_parse(const char *, size_t, bool, bool);

void subpaths_print(struct subpaths*, FILE*);

void subpath_free(struct subpath*);
void subpaths_release(struct subpaths*);
