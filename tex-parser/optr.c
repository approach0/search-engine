#include "head.h"

#undef N_DEBUG
#include <assert.h>

enum {
	depth_end,
	depth_begin,
	depth_going_end
};

static int depth_flag[MAX_OPTR_PRINT_DEPTH];
static bool gen_subpaths_bitmap[MAX_SUBPATH_ID * 2];

struct optr_node*
optr_alloc(enum symbol_id s_id, enum token_id t_id, bool comm)
{
	struct optr_node *n = malloc(sizeof(struct optr_node));

	n->commutative = comm; /* commutativity */
	n->wildcard = false; /* wildcard operand */
	n->symbol_id = s_id;
	n->token_id = t_id;
	n->sons = 0;
	n->rank = 0;
	n->subtr_hash = 0;
	n->path_id = 0;
	n->node_id = 0;
	n->pos_begin = 0;
	n->pos_end   = 0;
	TREE_NODE_CONS(n->tnd);
	return n;
}

static __inline__ void update(struct optr_node *c, struct optr_node *f)
{
	f->sons++;
	c->rank = f->sons;
}

static LIST_IT_CALLBK(pass_children_to_father)
{
	TREE_OBJ(struct optr_node, child, tnd);
	bool res;

	P_CAST(gf/* grandfather */,
	       struct optr_node, pa_extra);

	if (gf == NULL)
		return LIST_RET_BREAK; /* no children to pass */

	res = tree_detach(&child->tnd, pa_now, pa_fwd);

	tree_attach(&child->tnd, &gf->tnd, pa_now, pa_fwd);
	update(child, gf);

	return res;
}

struct optr_node* optr_attach(struct optr_node *c /* child */,
                              struct optr_node *f /* father */)
{
	if (c == NULL || f == NULL)
		return NULL;

	if (f->commutative && c->token_id == f->token_id) {
		/* apply commutative rule */
		list_foreach(&c->tnd.sons, &pass_children_to_father, f);
		free(c);
		return f;
	}

	tree_attach(&c->tnd, &f->tnd, NULL, NULL);
	update(c, f);

	return f;
}

char *optr_hash_str(symbol_id_t hash)
{
	static char str[sizeof(symbol_id_t) * 2 + 1];
	sprintf(str, "%04x", hash);
	return str;
}

static __inline__ void
print_node(FILE *fh, struct optr_node *p, bool is_leaf)
{
	struct optr_node *f =
		MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);

	fprintf(fh, "──");

	if (f && !f->commutative)
		fprintf(fh, "#%u", p->rank);

	if (is_leaf) {
		fprintf(fh, "[");
		fprintf(fh, C_GREEN "%s" C_RST, trans_symbol(p->symbol_id));

		if (p->wildcard)
			fprintf(fh, C_RED "(wildcard)" C_RST);
		fprintf(fh, "] ");
	} else {
		fprintf(fh, "(");
		fprintf(fh, C_BROWN "%s" C_RST, trans_symbol(p->symbol_id));
		fprintf(fh, ") ");

		// fprintf(fh, "%u son(s), ", p->sons);
	}

	fprintf(fh, "#%u, ", p->node_id);

	fprintf(fh, "token=%s, ", trans_token(p->token_id));
	//fprintf(fh, "path_id=%u, ", p->path_id);
	fprintf(fh, "subtr_hash=" C_GRAY "%s" C_RST ", ",
#if 0
		optr_hash_str(p->subtr_hash)
#else
		trans_symbol_wo_font(p->subtr_hash)
#endif
	);
	fprintf(fh, "pos=[%u, %u].", p->pos_begin, p->pos_end);
	fprintf(fh, "\n");
}

static TREE_IT_CALLBK(print)
{
	TREE_OBJ(struct optr_node, p, tnd);
	P_CAST(fh, FILE, pa_extra);
	int i;
	bool is_leaf;

	if (pa_now->now == pa_head->last)
	depth_flag[pa_depth] = depth_going_end;
	else if (pa_now->now == pa_head->now)
		depth_flag[pa_depth] = depth_begin;

	for (i = 0; i < pa_depth; i++) {
		switch (depth_flag[i + 1]) {
		case depth_end:
			fprintf(fh, "      ");
			break;
		case depth_begin:
			fprintf(fh, "     │");
			break;
		case depth_going_end:
			fprintf(fh, "     └");
			break;
		default:
			break;
		}
	}

	if (p->tnd.sons.now == NULL /* is leaf */)
		is_leaf = 1;
	else
		is_leaf = 0;

	print_node(fh, p, is_leaf);

	if (depth_flag[pa_depth] == depth_going_end)
		depth_flag[pa_depth] = depth_end;

	LIST_GO_OVER;
}

void optr_print(struct optr_node *optr, FILE *fh)
{
	if (optr == NULL)
		return;

//	uint32_t max_node_id = optr_max_node_id(optr);
//	printf("max node ID: %u\n", max_node_id);
	tree_foreach(&optr->tnd, &tree_pre_order_DFS,
	             &print, 0, fh);
}

static TREE_IT_CALLBK(release)
{
	TREE_OBJ(struct optr_node, p, tnd);
	bool res;
	res = tree_detach(&p->tnd, pa_now, pa_fwd);
	free(p);
	return res;
}

void optr_release(struct optr_node *optr)
{
	tree_foreach(&optr->tnd, &tree_post_order_DFS,
	             &release, 0, NULL);
}

static LIST_IT_CALLBK(digest_children)
{
	LIST_OBJ(struct optr_node, p, tnd.ln);
	P_CAST(children_hash, symbol_id_t, pa_extra);

	*children_hash += p->subtr_hash;

#ifdef OPTR_HASH_DEBUG
	printf("after digest %s: %s\n", trans_symbol(p->symbol_id),
	       optr_hash_str(*children_hash));
#endif

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(calpos_children)
{
	LIST_OBJ(struct optr_node, child, tnd.ln);
	P_CAST(father, struct optr_node, pa_extra);

	if (child->pos_begin < father->pos_begin)
		father->pos_begin = child->pos_begin;

	if (child->pos_end > father->pos_end)
		father->pos_end = child->pos_end;

	LIST_GO_OVER;
}

static TREE_IT_CALLBK(leafroot_path)
{
	TREE_OBJ(struct optr_node, p, tnd);

	if (p->tnd.sons.now == NULL /* is leaf */) {
		struct optr_node *q = p;
		while (q) {
			printf("%u ", q->node_id);
			q = MEMBER_2_STRUCT(q->tnd.father, struct optr_node, tnd);
		}
		q = p;
		while (q) {
			printf("%s ", trans_token(q->token_id));
			q = MEMBER_2_STRUCT(q->tnd.father, struct optr_node, tnd);
		}
		printf("\n");
	}
	LIST_GO_OVER;
}

void optr_leafroot_path(struct optr_node* optr)
{
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &leafroot_path,
	             0 /* excluding root */, NULL);
}

static TREE_IT_CALLBK(assign_value)
{
	struct optr_node *q;
	P_CAST(lcnt, uint32_t, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);
	symbol_id_t children_hash;

	if (p->tnd.sons.now == NULL /* is leaf */) {
		p->subtr_hash = p->symbol_id;
		p->path_id = ++(*lcnt);
		p->node_id = p->path_id;

		q = MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);
		while (q) {
			if (meaningful_gener(q->token_id) && q->path_id == 0) {
				q->path_id = p->path_id;
				break;
			}
			q = MEMBER_2_STRUCT(q->tnd.father, struct optr_node, tnd);
		}
	} else {
		/* generate subtree hash in DFS traversal */
		children_hash = 0;
		list_foreach(&p->tnd.sons, &digest_children, &children_hash);
		p->subtr_hash = p->symbol_id * children_hash;

		/* generate position range in DFS traversal */
		p->pos_begin = UINT32_MAX;
		p->pos_end = 0;
		list_foreach(&p->tnd.sons, &calpos_children, p);

#ifdef OPTR_HASH_DEBUG
		printf("%s * %s ", trans_symbol(p->symbol_id),
			   optr_hash_str(children_hash));
		printf("= %s\n", optr_hash_str(p->subtr_hash));
#endif
	}

	LIST_GO_OVER;
}

static TREE_IT_CALLBK(assign_node_id)
{
	P_CAST(lcnt, uint32_t, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);

	if (p->tnd.sons.now != NULL /* is not leaf */)
		p->node_id = ++(*lcnt);

	LIST_GO_OVER;
}

uint32_t optr_assign_values(struct optr_node *optr)
{
	uint32_t node_cnt, leaf_cnt = 0;
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &assign_value,
	             0 /* excluding root */, &leaf_cnt);
	node_cnt = leaf_cnt;
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &assign_node_id,
	             0 /* excluding root */, &node_cnt);

	/* return the maximum path_id assigned */
	return leaf_cnt;
}

static TREE_IT_CALLBK(purne_nil_node)
{
	TREE_OBJ(struct optr_node, p, tnd);
	P_CAST(pcnt, uint32_t, pa_extra);
	bool res;

	if (p->tnd.sons.now == NULL /* is leaf */ &&
	    p->tnd.father != NULL /* can be pruned */ &&
	    p->token_id == T_NIL) {

		/* prune this NIL node */
		res = tree_detach(&p->tnd, pa_now, pa_fwd);
		optr_release(p);

		(*pcnt)++;
		return res;
	}

	LIST_GO_OVER;
}

uint32_t optr_prune_nil_nodes(struct optr_node *optr)
{
	uint32_t purne_cnt = 0;
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &purne_nil_node,
	             0 /* excluding root */, &purne_cnt);

	return purne_cnt;
}

int is_single_node(struct optr_node *p)
{
	return (p->tnd.sons.now == NULL /* is leaf */ &&
	       p->tnd.father == NULL /* is root */);
}

struct subpath *create_subpath(struct optr_node *p, bool leaf)
{
	struct subpath *subpath = malloc(sizeof(struct subpath));

	subpath->path_id = p->path_id;
	subpath->leaf_id = p->node_id;
	LIST_CONS(subpath->path_nodes);

	/* a subpath is pseudo if it is created from expanding of
	 * another subpath, or if it is a wildcard path. */
	if (leaf) {
		subpath->type = (p->wildcard) ? \
		     SUBPATH_TYPE_WILDCARD : SUBPATH_TYPE_NORMAL;
		subpath->pseudo = (p->wildcard) ? true : false;
		subpath->lf_symbol_id = p->symbol_id;
	} else {
		subpath->type = SUBPATH_TYPE_GENERNODE;
		subpath->pseudo = false;
		subpath->subtree_hash = p->subtr_hash;
	}

	LIST_NODE_CONS(subpath->ln);

	return subpath;
}

static struct subpath_node *create_subpath_node(
	enum symbol_id symbol_id,
	enum token_id token_id,
	uint32_t sons,
	uint32_t node_id)
{
	struct subpath_node *nd;
	nd = malloc(sizeof(struct subpath_node));

	nd->symbol_id = symbol_id;
	nd->token_id = token_id;
	nd->sons = sons;
	nd->node_id = node_id;
	LIST_NODE_CONS(nd->ln);

	return nd;
}

struct _gen_fingerprint_arg {
	fingerpri_t fp;
	uint32_t cnt, prefix_len;
};

static LIST_IT_CALLBK(_gen_fingerprint)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(arg, struct _gen_fingerprint_arg, pa_extra);

	/* skip the first node (operand) */
	if (pa_now->now != pa_head->now) {

		/* prohibit to encode beyond 4 nodes */
		if (arg->cnt > 4) goto halt;

		arg->fp = arg->fp << 4;
		arg->fp = arg->fp | (sp_nd->symbol_id % 0xf);
	}

	if (arg->cnt >= arg->prefix_len - 1 ||
	    pa_now->now == pa_head->last) {
halt:
		return LIST_RET_BREAK;
	} else {
		arg->cnt ++;
		return LIST_RET_CONTINUE;
	}
}

fingerpri_t subpath_fingerprint(struct subpath *sp, uint32_t prefix_len)
{
	struct _gen_fingerprint_arg arg = {0};
	list_foreach(&sp->path_nodes, &_gen_fingerprint, &arg);
	return arg.fp;
}

struct _gen_subpaths_arg {
	struct subpaths *sp;
	uint32_t n_paths_limit;      /* constrain the number of paths */
	int lr_only; /* if only generate leaf-root paths */
};

/* Assign node-to-root path (from node p) to subpath path_nodes,
 * assign first node token, and create rank node if needed. */
void insert_subpath_nodes(struct subpath *subpath, struct optr_node *p,
                          enum token_id first_tok)
{
	struct subpath_node *nd;
	struct optr_node *f;
	uint32_t cnt = 0;

	do {
		f = MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);

		/* create and insert token node */
		if (cnt == 0)
			nd = create_subpath_node(p->symbol_id, first_tok,
			                         p->sons, p->node_id);
		else
			nd = create_subpath_node(p->symbol_id, p->token_id,
			                         p->sons, p->node_id);
		list_insert_one_at_tail(&nd->ln, &subpath->path_nodes, NULL, NULL);
		cnt ++; // increment subpath nodes counter

		/* create and insert rank node if necessary */
		if (f && !f->commutative) {
			/* should be OK to assign an zero ID for rank node, since
			 * rank node is NOT an interesting node (see interested_token()).
			 * As it will never be used as subr node nor a leaf node. */
			const uint32_t rank_node_id = 0;

			nd = create_subpath_node(
				S_NIL, T_MAX_RANK - (OPTR_INDEX_RANK_MAX - p->rank),
				1 /* rank node has only one son */, rank_node_id
			);

			list_insert_one_at_tail(&nd->ln, &subpath->path_nodes,
			                        NULL, NULL);
			cnt ++; // increment subpath nodes counter
		}

		p = f;
	} while (p);

	subpath->n_nodes = cnt;
}

static TREE_IT_CALLBK(gen_subpaths)
{
	P_CAST(arg, struct _gen_subpaths_arg, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);
	struct subpath   *subpath;
	struct optr_node *f;
	uint32_t          bitmap_idx;
	bool              is_leaf;

	if (p->tnd.sons.now == NULL /* is leaf */) {
		is_leaf = true;

		/* reached the limit of maximum paths we can generate */
		if (arg->sp->n_lr_paths >= arg->n_paths_limit)
			return LIST_RET_BREAK;
		else
			arg->sp->n_lr_paths ++; /* count leaf-root paths */

		do {
			f = MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);

			/* create a subpath from each node through bottom to top node */
			if (is_leaf || (!arg->lr_only &&
			    f != NULL && meaningful_gener(f->token_id))) {
				/* calculate bitmap index */
				if (is_leaf)
					bitmap_idx = p->path_id;
				else
					bitmap_idx = p->path_id + MAX_SUBPATH_ID;

				assert(bitmap_idx > 0);
				bitmap_idx --; /* map to [0, 63] */

				/* insert only when not inserted before */
				if (!gen_subpaths_bitmap[bitmap_idx]) {
					/* start create and generate a subpath */
					subpath = create_subpath(p, is_leaf);
					/* generate nodes of this subpath */
					insert_subpath_nodes(subpath, p, p->token_id);
					/* generate fingerprint of this subpath */
					subpath->fingerprint = subpath_fingerprint(subpath, UINT32_MAX);
					/* insert this new subpath to subpath list */
					list_insert_one_at_tail(&subpath->ln, &arg->sp->li,
					                        NULL, NULL);
					/* stop enter this if again */
					gen_subpaths_bitmap[bitmap_idx] = 1;
					/* count total subpaths generated. */
					arg->sp->n_subpaths ++;
				}
			}

			is_leaf = false;
			p = f;
		} while(p);

	}

	LIST_GO_OVER;
}

static TREE_IT_CALLBK(find_max_node_id)
{
	P_CAST(max_node_id, uint32_t, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);

	if (*max_node_id < p->node_id) {
		*max_node_id = p->node_id;
	}

	LIST_GO_OVER;
}

uint32_t optr_max_node_id(struct optr_node *optr)
{
	uint32_t max_node_id = 0;

	tree_foreach(&optr->tnd, &tree_post_order_DFS, &find_max_node_id,
	             0 /* including root */, &max_node_id);
	return max_node_id;
}

struct subpaths optr_subpaths(struct optr_node* optr, int lr_only)
{
	struct subpaths subpaths;
	struct _gen_subpaths_arg arg;
	LIST_CONS(subpaths.li);
	subpaths.n_lr_paths = 0;
	subpaths.n_subpaths = 0;

	/* clear bitmap */
	memset(gen_subpaths_bitmap, 0, sizeof(bool) * (MAX_SUBPATH_ID << 1));

	arg.sp = &subpaths;
	arg.n_paths_limit = MAX_SUBPATH_ID; /* generate limit number of paths */
	arg.lr_only = lr_only;
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &gen_subpaths,
	             0 /* including root */, &arg);
	return subpaths;
}

LIST_DEF_FREE_FUN(_subpath_nodes_release, struct subpath_node, ln,
	free(p)
);

void subpath_free(struct subpath *subpath)
{
	_subpath_nodes_release(&subpath->path_nodes);
	free(subpath);
}

LIST_DEF_FREE_FUN(_subpaths_release, struct subpath, ln,
	subpath_free(p);
);

void subpaths_release(struct subpaths *subpaths)
{
	_subpaths_release(&subpaths->li);
}

static __inline__ char *subpath_type_str(enum subpath_type t)
{
	static char type_map_str[][TYPE_MAP_STR_MAX] = {
		"gener",
		"wildcard",
		"normal",
	};

	return type_map_str[t];
}

static LIST_IT_CALLBK(print_subpath_path_node)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(fh, FILE, pa_extra);

	fprintf(fh, C_BROWN "%s" C_RST "(#%u)",
	        trans_token(sp_nd->token_id), sp_nd->node_id);

	if (pa_now->now != pa_head->last)
		fprintf(fh, "/");

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(print_subpath_list_item)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(fh, FILE, pa_extra);

	if (sp->type == SUBPATH_TYPE_GENERNODE)
		fprintf(fh, "* ");
	else if (sp->type == SUBPATH_TYPE_WILDCARD)
		fprintf(fh, "? ");
	else if (sp->type == SUBPATH_TYPE_NORMAL)
		fprintf(fh, "- ");
	else
		assert(0);

	fprintf(fh, "[path#%u, leaf#%u] ", sp->path_id, sp->leaf_id);

	if (sp->type == SUBPATH_TYPE_GENERNODE)
		fprintf(fh, C_BLUE "%s" C_RST,
		        optr_hash_str(sp->subtree_hash));
	else
		fprintf(fh, C_GREEN "%s" C_RST,
		        trans_symbol(sp->lf_symbol_id));

	fprintf(fh, ": ");
	list_foreach(&sp->path_nodes, &print_subpath_path_node, fh);
	fprintf(fh, " (fingerprint %s)", optr_hash_str(sp->fingerprint));
	fprintf(fh, "\n");

	LIST_GO_OVER;
}

void subpaths_print(struct subpaths *subpaths, FILE *fh)
{
	list_foreach(&subpaths->li, &print_subpath_list_item, fh);
}

struct _get_subpath_nodeid_at {
	uint32_t height;
	uint32_t cnt;
	uint32_t ret_node_id;
};

static LIST_IT_CALLBK(subpath_nodeid_at)
{
	LIST_OBJ(struct subpath_node, sp_nd, ln);
	P_CAST(args, struct _get_subpath_nodeid_at, pa_extra);

	args->cnt ++;
	args->ret_node_id = sp_nd->node_id;

	if (args->height == args->cnt) {
		return LIST_RET_BREAK;
	} else {
		LIST_GO_OVER;
	}
}

uint32_t get_subpath_nodeid_at(struct subpath *sp, uint32_t height)
{
	struct _get_subpath_nodeid_at args = {height, 0, 0};
	list_foreach(&sp->path_nodes, &subpath_nodeid_at, &args);

	return args.ret_node_id;
}

static TREE_IT_CALLBK(gen_idpos_map)
{
	P_CAST(map, uint32_t, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);

	if (p->tnd.sons.now == NULL /* is leaf */) {
		if (p->path_id < MAX_SUBPATH_ID) {
			map[p->path_id - 1] |= (p->pos_begin << 16);
			map[p->path_id - 1] |= (p->pos_end   << 0 );
		}
	}

	LIST_GO_OVER;
}

int optr_gen_idpos_map(uint32_t *map, struct optr_node *optr)
{
	/* clear bitmap */
	memset(map, 0, sizeof(uint32_t) * MAX_SUBPATH_ID);

	tree_foreach(&optr->tnd, &tree_post_order_DFS, &gen_idpos_map,
	             0 /* including root */, map);
	return 0;
}

int optr_print_idpos_map(uint32_t* map)
{
	for (int i = 0; i < MAX_SUBPATH_ID; i++) {
		if (map[i]) {
			uint32_t begin, end;
			begin = map[i] >> 16;
			end = map[i] & 0xffff;
			printf("pathID#%u: [%u, %u]\n", i + 1, begin, end);
		}
	}

	return 0;
}

struct _graph_pr_args {
	char **color;
	uint32_t *node_map;
	int K;
	sds *o;
};

static TREE_IT_CALLBK(graph_print)
{
	P_CAST(args, struct _graph_pr_args, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);

	struct optr_node *f =
		MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);

	char rank[128] = "";
	if (f && !f->commutative)
		sprintf(rank, "r%u", f->rank);

	*args->o = sdscatprintf(*args->o, "%u(#%u %s<br/>%s) \n",
		p->node_id, p->node_id, rank, trans_symbol(p->symbol_id));

	uint32_t n = args->node_map[p->node_id];
	if (n) {
		char *color = args->color[(n - 1) % args->K];
		*args->o = sdscatprintf(*args->o, "class %u "
			OPTR_TREE_NODE_COLOR_CSS_PREFIX "%s;\n", p->node_id, color);
		// *args->o = sdscatprintf(*args->o, "%%%% classDef %s%s fill:#f9f;\n",
		// 	OPTR_TREE_NODE_COLOR_CSS_PREFIX, color);
	}

	if (f) {
		/* non-root */
		*args->o = sdscatprintf(*args->o, "%u --> %u \n",
			f->node_id, p->node_id);
	}

	LIST_GO_OVER;
}

int optr_graph_print(struct optr_node* optr, char **colors,
                     uint32_t *node_map, int K, sds *o)
{
	struct _graph_pr_args args = {colors, node_map, K, o};
	*o = sdscatprintf(*o, "graph TD\n");
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &graph_print,
	             0 /* including root */, &args);
	return 0;
}
