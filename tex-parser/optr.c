#include <stdlib.h>
#include "optr.h"
#include "config.h"
#include "tex-parser.h"
#include "vt100-color.h"

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
	fprintf(fh, "fr_hash=" C_GRAY "%s" C_RST ", ", optr_hash_str(p->fr_hash));
	fprintf(fh, "ge_hash=" C_GRAY "%s" C_RST ".", optr_hash_str(p->ge_hash));
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

void optr_print(struct optr_node *tr, FILE *fh)
{
	if (tr == NULL)
		return;
	tree_foreach(&tr->tnd, &tree_pre_order_DFS,
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

void optr_release(struct optr_node *tr)
{
	tree_foreach(&tr->tnd, &tree_post_order_DFS,
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

static TREE_IT_CALLBK(assign_ge_hash)
{
	TREE_OBJ(struct optr_node, p, tnd);
	symbol_id_t children_hash;

	if (p->tnd.sons.now == NULL /* is leaf */) {
		p->ge_hash = p->symbol_id;
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

void optr_ge_hash(struct optr_node *tr)
{
	tree_foreach(&tr->tnd, &tree_post_order_DFS, &assign_ge_hash,
	             1 /* excluding root */, NULL);
}

#if 0

static TREE_IT_CALLBK(gen_subpath)
{
	TREE_OBJ(struct tex_tr, p, tnd);
	P_CAST(gsa, struct gen_subpath_arg, pa_extra);
	struct tex_tr *last_p = NULL;
	bool  reach_rot = 0;
	struct subpath *sp;
	uint32_t i;
	char *d;

	if (p->tnd.sons.now == NULL /* is leaf */ &&
	    p->n_fan != 0 /* not grouped */) {
		
		/* limit checks before allocate new subpath */
		if (p->symbol_id > MAX_BRW_SYMBOL_ID_SZ) {
			trace(LIMIT, "%d > max symol id sz\n", p->symbol_id);

			gsa->err = 1;
			return LIST_RET_BREAK;
		} else if (p->node_id > MAX_BRW_PIN_SZ ||
		           p->n_fan > MAX_BRW_FAN_SZ) {
			trace(LIMIT, "node_id, n_fan (%d, %d) exceeds limit.\n", 
			      p->node_id, p->n_fan);

			gsa->err = 1;
			return LIST_RET_BREAK;
		} else if (pa_depth >= MAX_TEX_TR_DEPTH) {
			trace(LIMIT, "%d >= max textr depth.\n", pa_depth);

			gsa->err = 1;
			return LIST_RET_BREAK;
		}

		/* a new subpath being added */
		sp = malloc(sizeof(struct subpath));
		sp->dir[0] = '\0';
		LIST_NODE_CONS(sp->ln);
		d = sp->dir;
		d += sprintf(d, "./");

		{ /* limit already checked */
			sp->brw.symbol_id = p->symbol_id;
		}

		i = 0;

		/* go up through subpath from leaf to specified root */
		do {
#if (DEBUG_TEX_TR_PRINT_ID)
			if (is_sequential(p->token_id) && last_p) {
				d += sprintf(d, "%d_%d", p->token_id, last_p->rank);
			} else {
				d += sprintf(d, "%d", p->token_id);
			}
#else
			if (is_sequential(p->token_id) && last_p) {
				d += sprintf(d, "%s_%d", trans_token(p->token_id), 
				             last_p->rank);
			} else {
				d += sprintf(d, "%s", trans_token(p->token_id));
			}
#endif

			{ /* limit already checked */
				sp->brw.pin[i] = p->node_id;
				sp->brw.fan[i] = p->n_fan;
			}

			if (p == gsa->rot)
				reach_rot = 1;
			else
				d += sprintf(d, "/");

			last_p = p;
			p = MEMBER_2_STRUCT(p->tnd.father, struct tex_tr, tnd);

			if (p == NULL) /* safe guard */
				reach_rot = 1;

			i ++;
		} while (!reach_rot);
		
		/* append a zero at the end */
		sp->brw.pin[i] = 0;
		sp->brw.fan[i] = 0;
		
		/* update max_depth statistics */
		if (i >= MAX_TEX_TR_DEPTH) {
			trace(LIMIT, "exceeding depth %d after checking %d.\n", 
			      i, pa_depth);
			CP_FATAL;
		}

		list_insert_one_at_tail(&sp->ln, gsa->li, NULL, NULL);
	}

	LIST_GO_OVER;
}

/* 
 * This function generates branch words to be written 
 * in the posting file, it is crucial to include some 
 * limits check before the subpaths are written into
 * posting file. 
 */
struct list_it tex_tr_subpaths(struct tex_tr *tr, int *err)
{
	struct list_it li = LIST_NULL;
	struct gen_subpath_arg gsa = {&li, tr, 0};

	tree_foreach(&tr->tnd, &tree_post_order_DFS, 
	             &gen_subpath, 0, &gsa);

	*err = gsa.err;
	return li;
}

LIST_DEF_FREE_FUN(subpaths_free, struct subpath, ln, free(p));

static LIST_IT_CALLBK(subpath_print)
{
	LIST_OBJ(struct subpath, p, ln);
	P_CAST(fh, FILE, pa_extra);

	fprintf(fh, "%s ", p->dir);
	print_brw(&p->brw, fh);
	fprintf(fh, "\n");

	LIST_GO_OVER;
}

void subpaths_print(struct list_it *li, FILE *fh)
{
	list_foreach(li, &subpath_print, fh);
}
#endif
