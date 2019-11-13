#include <stdint.h>
#include "config.h"
#include "math-qry.h"
#include "math-pruning.h"
#include "mnc-score.h"

/* math level-2 inverted list */
struct math_l2_invlist {
	struct math_qry mq;
	struct math_score_factors msf;
	float *threshold;
};

struct math_l2_invlist *math_l2_invlist(math_index_t, const char*, float*);
void  math_l2_invlist_free(struct math_l2_invlist*);

/* iterator */
typedef struct ms_merger *merger_set_iter_t;

struct math_l2_iter_item {
	uint32_t docID;
	float    score;
	uint32_t n_occurs, occur[MAX_MATH_OCCURS];
};

typedef struct math_l2_invlist_iter {
	/* copied from level-2 invert list */
	int n_qnodes;
	float  *ipf;
	struct math_score_factors *msf;
	FILE **fh_symbinfo;       /* used for symbol scoring */
	struct subpath_ele **ele; /* used for symbol scoring */
	struct mnc_scorer *mnc; /* symbol scorer */
	const char *tex;

	/* merger and pruner for level-2 merge */
	merger_set_iter_t merge_iter;
	struct math_pruner *pruner;

	/* item book-keeping */
	struct math_l2_iter_item item;
	uint32_t future_docID;
	float  last_threshold, *threshold;
} *math_l2_invlist_iter_t;

/* iterator functions */
math_l2_invlist_iter_t math_l2_invlist_iterator(struct math_l2_invlist*);
void     math_l2_invlist_iter_free(math_l2_invlist_iter_t);
uint64_t math_l2_invlist_iter_cur(math_l2_invlist_iter_t);
int      math_l2_invlist_iter_next(math_l2_invlist_iter_t);
size_t   math_l2_invlist_iter_read(math_l2_invlist_iter_t, void*, size_t);

/* level-2 inverted list upperbound calculation */
float math_l2_invlist_iter_upp(math_l2_invlist_iter_t);
