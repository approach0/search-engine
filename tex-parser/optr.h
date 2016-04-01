#include "gen-token.h"
#include "gen-symbol.h"
#include "trans.h"

#include "tree/tree.h"
#include "stdbool.h"

#define WC_NORMAL_LEAF      0
#define WC_WILDCD_LEAF      1
#define WC_NONCOM_OPERATOR  0
#define WC_COMMUT_OPERATOR  1

struct optr_node {
	union {
		bool         commutative;
		bool         wildcard;
	};
	enum symbol_id   symbol_id;
	enum token_id    token_id;
	uint32_t         sons;
	uint32_t         rank;
	uint8_t          br_hash;
	struct tree_node tnd;
};

struct optr_node* optr_alloc(enum symbol_id, enum token_id, bool);

struct optr_node* optr_attach(struct optr_node*, struct optr_node*);

void optr_print(struct optr_node*, FILE*);

void optr_release(struct optr_node*);
