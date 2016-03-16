#include "enum-token.h"
#include "enum-symbol.h"
#include "../tree/tree.h"

struct tex_tr {
	uint32_t          node_id;
	enum token_id     token_id;
	enum symbol_id    symbol_id;
	uint32_t          n_fan;
	uint32_t          rank; /* used to assign son's rank */
	struct tree_node  tnd;
};

struct tex_tr *tex_tr_alloc(enum symbol_id, enum token_id);

void tex_tr_attatch(struct tex_tr*, struct tex_tr*);

/*
uint32_t tex_tr_assign(struct tex_tr*);

uint32_t tex_tr_prune(struct tex_tr*);

void tex_tr_group(struct tex_tr*);

uint32_t tex_tr_update(struct tex_tr*);

void tex_tr_print(struct tex_tr*, FILE*);

void tex_tr_release();

struct list_it tex_tr_subpaths(struct tex_tr*, int*);

void subpaths_print(struct list_it*, FILE*);

LIST_DECL_FREE_FUN(subpaths_free);
*/
