#include "math-index-v3/subpath-set.h" /* for subpath_ele */
#include "merger/mergers.h"
#include "config.h"
#include "bin-lp.h"

struct math_pruner_qnode {
	int root, sum_w;
	float sum_ipf;
	int n; /* number of sector trees */
	int secttr_w[MAX_MERGE_SET_SZ];
	int invlist_id[MAX_MERGE_SET_SZ];
};

struct math_pruner_backref {
	int  *idx; /* reference sector tree qnodes[idx] */
	int  *ref; /* width of reference sector tree */
	int   cnt; /* back-reference counter */
	int   max; /* MaxRef */
};

struct math_pruner {
	struct math_qry *mq; /* subject query */
	void *score_judger;
	float threshold_; /* updated threshold for internal use */

	/* query nodes */
	struct math_pruner_qnode *qnodes;
	uint32_t n_qnodes;

	/* back references: mapping from [invlist ID] */
	struct math_pruner_backref backrefs[MAX_MERGE_SET_SZ];

	struct bin_lp blp; /* binary linear-programming problem */
};

struct math_qry;

struct math_pruner
    *math_pruner_init(struct math_qry *, void *, float);
void math_pruner_free(struct math_pruner*);
int  math_pruner_update(struct math_pruner*, float);
void math_pruner_print(struct math_pruner*);

void math_pruner_iters_drop(struct math_pruner *, struct ms_merger *);
void math_pruner_iters_sort_by_maxref(struct math_pruner*, struct ms_merger*);
void math_pruner_iters_gbp_assign(struct math_pruner*, struct ms_merger*, int);
