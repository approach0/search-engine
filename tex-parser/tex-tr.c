#include <stdlib.h>
#include "tex-tr.h"
#include "../include/config.h"
#include "trans.h"

struct tex_tr *
tex_tr_alloc(enum symbol_id s_id, enum token_id t_id)
{
	struct tex_tr *n = malloc(sizeof(struct tex_tr));
	n->node_id = 0;
	n->token_id = t_id;
	n->symbol_id = s_id;
	n->n_fan = 0;
	n->rank = 0;
	TREE_NODE_CONS(n->tnd);
	return n;
}

static LIST_IT_CALLBK(pass_children_to_father)
{
	TREE_OBJ(struct tex_tr, son, tnd);
	bool res;

	P_CAST(gf/* grandfather */, struct tex_tr, pa_extra); 

	if (gf == NULL)
		return LIST_RET_BREAK;

	res = tree_detach(&son->tnd, pa_now, pa_fwd);
	tree_attach(&son->tnd, &gf->tnd, pa_now, pa_fwd);

	return res;
}

bool is_sequential(enum token_id id) 
{
	return (id == T_CHOOSE || id == T_SQRT || id == T_FRAC ||
	        id == T_MODULAR);
}

bool is_commutative(enum token_id id) 
{
	return (id == T_ADD || id == T_TIMES || id == T_TAB || 
	        id == T_SEP_CLASS || id == T_HANGER);
}

void tex_tr_attatch(struct tex_tr *f /* father */, 
                    struct tex_tr *s /* son */)
{
	if (f == NULL || s == NULL)
		return;

	if (s->token_id == f->token_id) {
		if (is_commutative(f->token_id)) {
			/* token with associative law */
			list_foreach(&s->tnd.sons, 
			             &pass_children_to_father, f);
			free(s);
		}
		else {
			goto attach;
		}
	} else {
attach:
		tree_attach(&s->tnd, &f->tnd, NULL, NULL);
	}
}

static int depth_flag[MAX_TEX_TR_DEPTH];

enum {
	depth_end,
	depth_begin,
	depth_going_end
};

static TREE_IT_CALLBK(print)
{
	TREE_OBJ(struct tex_tr, p, tnd);
	P_CAST(fh, FILE, pa_extra);
	int i;
	bool is_leaf;
	
	if (p->tnd.sons.now == NULL /* is leaf */)
		is_leaf = 1;
	else
		is_leaf = 0;

	if (pa_now->now == pa_head->last)
	depth_flag[pa_depth] = depth_going_end;
	else if (pa_now->now == pa_head->now)
		depth_flag[pa_depth] = depth_begin;
	
	for (i = 0; i<pa_depth; i++) { 
		switch (depth_flag[i + 1]) {
		case depth_end:
			fprintf(fh, "    ");
			break;
		case depth_begin:
			fprintf(fh, "   │");
			break;
		case depth_going_end:
			fprintf(fh, "   └");
			break;
		default:
			break;
		}
	}
	
	if (is_leaf) 
		fprintf(fh, "──[");
	else 
		fprintf(fh, "──(");

#if (DEBUG_TEX_TR_PRINT_ID)
	fprintf(fh, "s%d, t%d, #%d, *%d, @%d", p->symbol_id, 
	        p->token_id, p->node_id, p->n_fan, p->rank);
#else
	fprintf(fh, "%s, %s, #%d, *%d, @%d", 
	        trans_symbol(p->symbol_id), 
	        trans_token(p->token_id), 
	        p->node_id, p->n_fan, p->rank);
#endif

	if (is_leaf) 
		fprintf(fh, "]");
	else 
		fprintf(fh, ")");
	
	if (p->n_fan == 0 && is_leaf)
		fprintf(fh, " (grouped)");
	
	fprintf(fh, "\n");

	if (depth_flag[pa_depth] == depth_going_end)
		depth_flag[pa_depth] = depth_end;
	LIST_GO_OVER;
}

void tex_tr_print(struct tex_tr *tr, FILE *fh)
{
	tree_foreach(&tr->tnd, &tree_pre_order_DFS, 
	             &print, 0, fh);
}

#if 0
static TREE_IT_CALLBK(release)
{
	TREE_OBJ(struct tex_tr, p, tnd);
	bool res;
	res = tree_detach(&p->tnd, pa_now, pa_fwd);
	free(p);
	return res;
}

void tex_tr_release(struct tex_tr *tr)
{
	tree_foreach(&tr->tnd, &tree_post_order_DFS, 
	             &release, 0, NULL);
}

static LIST_IT_CALLBK(count_sons)
{
	TREE_OBJ(struct tex_tr, p, tnd);
	P_CAST(cnt, uint32_t, pa_extra);
	p->rank = (*cnt) ++;
	LIST_GO_OVER;
}

struct assign_id_arg {
	uint32_t cur_id; /* current ID to be assigned */
	uint32_t tot_brw; /* total branch words, including grouped */
};

static TREE_IT_CALLBK(assign_leaf_id)
{
	TREE_OBJ(struct tex_tr, p, tnd);
	P_CAST(aia, struct assign_id_arg, pa_extra);

	if (p->tnd.sons.now == NULL /* is leaf */) {
		p->node_id = ++(aia->cur_id);
		aia->tot_brw += p->n_fan;
	}

	LIST_GO_OVER;
}

static TREE_IT_CALLBK(assign_internal)
{
	TREE_OBJ(struct tex_tr, p, tnd);
	P_CAST(aia, struct assign_id_arg, pa_extra);
	uint32_t sons;

	if (p->tnd.sons.now != NULL /* is internal */ &&
	    p->n_fan == 0 /* not assigned */) {
		/* assign id */
		p->node_id = ++(aia->cur_id); /* assign node id */

		/* assign n_fan ant the rank of sons */
		sons = 0;
		list_foreach(&p->tnd.sons, &count_sons, &sons);
		p->n_fan = sons;
	}

	LIST_GO_OVER;
}

/* assign id/fan/rank to tree nodes. */
uint32_t tex_tr_assign(struct tex_tr *tr)
{
	struct assign_id_arg aia = {0, 0};

	tree_foreach(&tr->tnd, &tree_post_order_DFS, 
	             &assign_leaf_id, 0, &aia);
	
	tree_foreach(&tr->tnd, &tree_post_order_DFS, 
	             &assign_internal, 0, &aia);

	return aia.tot_brw;
}

static TREE_IT_CALLBK(leaf_n_fan_set_1)
{
	TREE_OBJ(struct tex_tr, p, tnd);

	if (p->tnd.sons.now == NULL /* is leaf */)
		p->n_fan = 1;
	
	LIST_GO_OVER;
}

static LIST_IT_CALLBK(update_same_brothers)
{
	TREE_OBJ(struct tex_tr, bro, tnd);
	P_CAST(l, struct tex_tr, pa_extra);

	if (bro != l && bro->symbol_id == l->symbol_id) {
		l->n_fan ++;
		bro->n_fan = 0; /* mark it as grouped */
	}

	LIST_GO_OVER;
}

static void leaf_group(struct tex_tr *l)
{
	struct tex_tr *f /* father */
	     = MEMBER_2_STRUCT(l->tnd.father, struct tex_tr, tnd);

	/* 
	 * for leaf, n_fan means the number of brothers 
	 * with same symbol_id. 
	 */
	list_foreach(&f->tnd.sons, &update_same_brothers, l);
}


static TREE_IT_CALLBK(group_leaf)
{
	TREE_OBJ(struct tex_tr, p, tnd);

	if (p->tnd.sons.now == NULL /* is leaf */ &&
	    p->n_fan != 0 /* not grouped */) {
		if (NULL != p->tnd.father)
			leaf_group(p);
	}

	LIST_GO_OVER;
}

void tex_tr_group(struct tex_tr *tr) 
{
	tree_foreach(&tr->tnd, &tree_post_order_DFS, 
	             &leaf_n_fan_set_1, 0, NULL);
	tree_foreach(&tr->tnd, &tree_post_order_DFS, 
	             &group_leaf, 0, NULL);
}

static TREE_IT_CALLBK(prune)
{
	TREE_OBJ(struct tex_tr, p, tnd);
	P_CAST(n_pruned, uint32_t, pa_extra);
	bool res;

	if (p->tnd.sons.now == NULL /* is leaf */ &&
	    NULL != p->tnd.father /* can be pruned */) {
		/* prune ... */
		if (p->n_fan == 0 /* grouped nodes */ ||
		    p->token_id == T_NIL /* nil nodes */) {
			res = tree_detach(&p->tnd, pa_now, pa_fwd);	
			tex_tr_release(p);
			++(*n_pruned);
			return res;
		}
	}

	LIST_GO_OVER;
}

uint32_t tex_tr_prune(struct tex_tr *tr)
{
/* prune nil and grouped nodes, and
 * return number of nodes pruned. */
	uint32_t n_pruned;
	tree_foreach(&tr->tnd, &tree_post_order_DFS, 
	             &prune, 0, &n_pruned);
	return n_pruned;
}

uint32_t tex_tr_update(struct tex_tr *tr)
{
	uint32_t n_leaves;
	tex_tr_group(tr);
	tex_tr_prune(tr);
	n_leaves = tex_tr_assign(tr);

	if (n_leaves > MAX_TEX_TR_LEAVES) {
		trace(LIMIT, "%d > max leaves\n", n_leaves);
		return 0;
	}

	return n_leaves;
}

struct gen_subpath_arg {
	struct list_it *li;
	struct tex_tr  *rot;
	int             err;
};

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
