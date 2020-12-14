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
	uint32_t         sum_class;
	uint32_t         tex_braced;
	symbol_id_t      subtr_hash;
	uint32_t         path_id;
	uint32_t         node_id;
	uint32_t         pos_begin, pos_end;
	struct tree_node tnd;
};

struct optr_node* optr_alloc(enum symbol_id, enum token_id, bool);
struct optr_node* optr_copy(struct optr_node*);

struct optr_node* optr_pass_children(struct optr_node*, struct optr_node*);
struct optr_node* optr_attach(struct optr_node*, struct optr_node*);

void optr_print(struct optr_node*, FILE*);

void optr_release(struct optr_node*);

char *optr_hash_str(symbol_id_t);

uint32_t optr_assign_values(struct optr_node*);

uint32_t optr_prune_nil_nodes(struct optr_node*);

struct subpaths optr_lrpaths(struct optr_node*);

void optr_leafroot_path(struct optr_node*);

uint32_t optr_max_node_id(struct optr_node*);

int optr_gen_idpos_map(uint32_t*, struct optr_node*);

int optr_gen_visibi_map(uint32_t*, struct optr_node*);

int optr_print_idpos_map(uint32_t*);

int optr_print_visibi_map(uint32_t*);

int is_single_node(struct optr_node*);

fingerpri_t fingerprint(struct subpath*, uint32_t);

char *optr_hash_str(symbol_id_t);

#include "sds/sds.h"
int
optr_graph_print(struct optr_node*, char **, uint32_t*, int, sds*);
