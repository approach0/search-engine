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
	float *dynm_threshold /* dynamic threshold */;
};

struct math_l2_invlist
*math_l2_invlist(math_index_t, const char*, float*, float*);

void  math_l2_invlist_free(struct math_l2_invlist*);

/* iterator read item */
struct math_l2_iter_item {
	uint32_t docID;
	float    score;
	uint32_t n_occurs, occur[MAX_MATH_OCCURS];
};

/* iterator */
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
	struct ms_merger *merge_iter;
	struct math_pruner *pruner;

	/* item book-keeping */
	struct math_l2_iter_item item;
	uint32_t real_curID; /* docs before real_curID are passed */
	float  last_threshold, *threshold;
	float *dynm_threshold;
} *math_l2_invlist_iter_t;

/* iterator functions */
math_l2_invlist_iter_t math_l2_invlist_iterator(struct math_l2_invlist*);
void     math_l2_invlist_iter_free(math_l2_invlist_iter_t);
uint64_t math_l2_invlist_iter_cur(math_l2_invlist_iter_t);
int      math_l2_invlist_iter_next(math_l2_invlist_iter_t);
int      math_l2_invlist_iter_skip(math_l2_invlist_iter_t, uint64_t);
size_t   math_l2_invlist_iter_read(math_l2_invlist_iter_t, void*, size_t);

/* level-2 inverted list upperbound calculation */
float math_l2_invlist_iter_upp(math_l2_invlist_iter_t);
