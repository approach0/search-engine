#define WC_NONCOM_OPERATOR  0
#define WC_COMMUT_OPERATOR  1
#define WC_NORMAL_LEAF      1

struct optr_node {
	bool             commutative;
	bool             wildcard;
	enum symbol_id   symbol_id;
	enum token_id    token_id;
	uint32_t         sons;
	uint32_t         rank;
	uint32_t         always_base;
	symbol_id_t      subtr_hash;
	uint32_t         path_id;
	uint32_t         node_id;
	uint32_t         pos_begin, pos_end;
	struct tree_node tnd;
};

struct optr_node* optr_alloc(enum symbol_id, enum token_id, bool);
struct optr_node* optr_copy(struct optr_node*);

struct optr_node* optr_attach(struct optr_node*, struct optr_node*);

void optr_print(struct optr_node*, FILE*);

void optr_release(struct optr_node*);

char *optr_hash_str(symbol_id_t);

uint32_t optr_assign_values(struct optr_node*);

uint32_t optr_prune_nil_nodes(struct optr_node*);

struct subpaths optr_subpaths(struct optr_node*, int);

void optr_leafroot_path(struct optr_node*);

uint32_t optr_max_node_id(struct optr_node*);

int optr_gen_idpos_map(uint32_t*, struct optr_node*);

int optr_gen_visibi_map(uint32_t*, struct optr_node*);

int optr_print_idpos_map(uint32_t*);

int optr_print_visibi_map(uint32_t*);

void insert_subpath_nodes(struct subpath*, struct optr_node*, enum token_id);

struct subpath *create_subpath(struct optr_node*, bool);

int is_single_node(struct optr_node*);

fingerpri_t subpath_fingerprint(struct subpath*, uint32_t);
char *optr_hash_str(symbol_id_t);

#include "sds/sds.h"
int
optr_graph_print(struct optr_node*, char **, uint32_t*, int, sds*);
