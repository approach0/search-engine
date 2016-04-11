#include "head.h"

struct optr_node* optr_alloc(enum symbol_id s_id, enum token_id t_id, bool uwc)
{
	struct optr_node *n = malloc(sizeof(struct optr_node));

	n->wildcard = uwc;
	n->symbol_id = s_id;
	n->token_id = t_id;
	n->sons = 0;
	n->rank = 0;
	n->fr_hash = s_id;
	n->ge_hash = 0;
	n->path_id = 0;
	TREE_NODE_CONS(n->tnd);
	return n;
}

static __inline__ void update(struct optr_node *c, struct optr_node *f)
{
	tree_attach(&c->tnd, &f->tnd, NULL, NULL);
	f->sons++;
	c->rank = f->sons;
	c->fr_hash = f->fr_hash + c->symbol_id;
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

static int depth_flag[MAX_OPTR_DEPTH];

enum {
	depth_end,
	depth_begin,
	depth_going_end
};

char *optr_hash_str(symbol_id_t hash)
{
#define LEN (sizeof(symbol_id_t))
	int i;
	static char str[LEN * 2 + 1];
	uint8_t *c = (uint8_t*)&hash;
	str[LEN * 2] = '\0';
	
	for (i = 0; i < LEN; i++, c++) {
		sprintf(str + (i << 1), "%02x", *c);
	}

	return str;
}

static __inline__ void
print_node(FILE *fh, struct optr_node *p, bool is_leaf)
{
	struct optr_node *f = MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);

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

		fprintf(fh, "%u son(s), ", p->sons);
	}

	fprintf(fh, "token=%s, ", trans_token(p->token_id));
	fprintf(fh, "path_id=%u, ", p->path_id);
	fprintf(fh, "ge_hash=" C_GRAY "%s" C_RST ", ", optr_hash_str(p->ge_hash));
	fprintf(fh, "fr_hash=" C_GRAY "%s" C_RST ".", optr_hash_str(p->fr_hash));
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
	printf("Operator tree:\n");
	if (optr == NULL)
		return;

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

	*children_hash += p->symbol_id;

#ifdef OPTR_HASH_DEBUG
	printf("after digest %s: %s\n", trans_symbol(p->symbol_id),
	       optr_hash_str(*children_hash));
#endif

	LIST_GO_OVER;
}

static TREE_IT_CALLBK(assign_value)
{
	struct optr_node *q;
	P_CAST(lcnt, uint32_t, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);
	symbol_id_t children_hash;

	if (p->tnd.sons.now == NULL /* is leaf */) {
		p->ge_hash = p->symbol_id;
		p->path_id = ++(*lcnt);

		q = MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);
		while (q) {
			if (q->sons > 1 && q->path_id == 0) {
				q->path_id = p->path_id;
				break;
			}
			q = MEMBER_2_STRUCT(q->tnd.father, struct optr_node, tnd);
		}
	} else {
		children_hash = 0;
		list_foreach(&p->tnd.sons, &digest_children, &children_hash);
		p->ge_hash = p->symbol_id * children_hash;

#ifdef OPTR_HASH_DEBUG
		printf("%s * %s ", trans_symbol(p->symbol_id),
			   optr_hash_str(children_hash));
		printf("= %s\n", optr_hash_str(p->ge_hash));
#endif
	}

	LIST_GO_OVER;
}

void optr_assign_values(struct optr_node *optr)
{
	uint32_t leaf_cnt = 0;
	tree_foreach(&optr->tnd, &tree_post_order_DFS, &assign_value,
	             1 /* excluding root */, &leaf_cnt);
}

struct subpath *create_subpath(struct optr_node *p, bool leaf)
{
	struct subpath *subpath = malloc(sizeof(struct subpath));

	subpath->path_id = p->path_id;
	LIST_CONS(subpath->path_nodes);

	if (leaf) {
		subpath->type = (p->wildcard) ? \
		     SUBPATH_TYPE_WILDCARD : SUBPATH_TYPE_NORMAL;
		subpath->lf_symbol_id = p->symbol_id;
	} else {
		subpath->type = SUBPATH_TYPE_GENERNODE;
		subpath->ge_hash = p->ge_hash;
	}

	subpath->fr_hash = p->fr_hash;
	LIST_NODE_CONS(subpath->ln);

	return subpath;
}

struct subpath_node *create_subpath_node(enum token_id token_id, uint32_t sons)
{
	struct subpath_node *nd;
	nd = malloc(sizeof(struct subpath_node));
	nd->token_id = token_id;
	nd->sons = sons;
	LIST_NODE_CONS(nd->ln);

	return nd;
}

void insert_subpath_nodes(struct subpath *subpath, struct optr_node *p)
{
	struct subpath_node *nd;
	struct optr_node *f;

	do {
		f = MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);

		/* create and insert token node */
		nd = create_subpath_node(p->token_id, p->sons);
		list_insert_one_at_tail(&nd->ln, &subpath->path_nodes, NULL, NULL);

		/* create and insert rank node if necessary */
		if (f && !f->commutative) {
			nd = create_subpath_node(T_MAX_RANK - (OPTR_INDEX_RANK_MAX - p->rank), 1);
			list_insert_one_at_tail(&nd->ln, &subpath->path_nodes, NULL, NULL);
		}

		p = f;
	} while(p);
}

static bool gen_subpaths_bitmap[MAX_SUBPATH_ID * 2];

static TREE_IT_CALLBK(gen_subpaths)
{
	P_CAST(ret, struct subpaths, pa_extra);
	TREE_OBJ(struct optr_node, p, tnd);
	struct subpath   *subpath;
	struct optr_node *f;
	uint32_t          bitmap_idx;
	bool              is_leaf;

	if (p->tnd.sons.now == NULL /* is leaf */) {
		is_leaf = true;

		/* count leaf-root paths */
		ret->n_lr_paths ++;

		do {
			f = MEMBER_2_STRUCT(p->tnd.father, struct optr_node, tnd);

			/* create a subpath */
			if (is_leaf || (p->sons > 1 && f != NULL)) {
				/* calculate bitmap index */
				if (is_leaf)
					bitmap_idx = p->path_id;
				else
					bitmap_idx = p->path_id + MAX_SUBPATH_ID;

				/* insert only when not inserted before (by looking at bitmap)  */
				if (!gen_subpaths_bitmap[bitmap_idx]) {
					subpath = create_subpath(p, is_leaf);
					insert_subpath_nodes(subpath, p);
					list_insert_one_at_tail(&subpath->ln, &ret->li, NULL, NULL);
					gen_subpaths_bitmap[bitmap_idx] = 1;
				}
			}

			is_leaf = false;
			p = f;
		} while(p);
	
	}
	
	LIST_GO_OVER;
}

struct subpaths optr_subpaths(struct optr_node* optr)
{
	struct subpaths subpaths;
	LIST_CONS(subpaths.li);
	subpaths.n_lr_paths = 0;

	/* clear bitmap */
	memset(gen_subpaths_bitmap, 0, sizeof(bool) * (MAX_SUBPATH_ID << 1));

	tree_foreach(&optr->tnd, &tree_post_order_DFS, &gen_subpaths,
	             1 /* excluding root */, &subpaths);
	return subpaths;
}

LIST_DEF_FREE_FUN(_subpath_nodes_release, struct subpath_node, ln, free(p));

LIST_DEF_FREE_FUN(_subpaths_release, struct subpath, ln,
	_subpath_nodes_release(&p->path_nodes);
	free(p);
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

	fprintf(fh, C_BROWN "%s" C_RST "(%u)/", trans_token(sp_nd->token_id), sp_nd->sons);

	LIST_GO_OVER;
}

static LIST_IT_CALLBK(print_subpath_list_item)
{
	LIST_OBJ(struct subpath, sp, ln);
	P_CAST(fh, FILE, pa_extra); 

	fprintf(fh, "* ");
	list_foreach(&sp->path_nodes, &print_subpath_path_node, fh);

	fprintf(fh, "[");
	fprintf(fh, "path_id=%u: type=%s, ", sp->path_id, subpath_type_str(sp->type));

	if (sp->type == SUBPATH_TYPE_GENERNODE)
		fprintf(fh, "ge_hash=" C_GRAY "%s" C_RST ", ", 
		        optr_hash_str(sp->ge_hash));
	else
		fprintf(fh, "leaf symbol=" C_GREEN "%s" C_RST ", ", 
		        trans_symbol(sp->lf_symbol_id));

	fprintf(fh, "fr_hash=" C_GRAY "%s" C_RST, optr_hash_str(sp->fr_hash));
	
	fprintf(fh, "]\n");

	LIST_GO_OVER;
}

void subpaths_print(struct subpaths *subpaths, FILE *fh)
{
	printf("Subpaths (%u leaf-root paths):\n", subpaths->n_lr_paths);
	list_foreach(&subpaths->li, &print_subpath_list_item, fh);
}
