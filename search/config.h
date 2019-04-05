/* for MAX_TXT_SEG_BYTES & MAX_TXT_SEG_LEN */
#include "txt-seg/config.h"

/* consider both math & term */
#define MAX_QUERY_BYTES     (MAX_TXT_SEG_BYTES * 32)
#define MAX_QUERY_WSTR_LEN  (MAX_TXT_SEG_LEN * 32)

#define SNIPPET_PADDING    320
#define MAX_TEX_STRLEN    (MAX_TXT_SEG_BYTES * 32)
#define MAX_SNIPPET_SZ     8192

//#define DEBUG_SNIPPET

/* max mark score, type of mnc_score_t */
#define MNC_MARK_BASE_SCORE 90
#define MNC_MARK_FULL_SCORE 100
#define MNC_MARK_MID_SCORE ((MNC_MARK_BASE_SCORE + MNC_MARK_FULL_SCORE) / 2)

#define MATH_SYMBOLIC_SCORING_ENABLE /* enable symbolic scoring */
#define MATH_PRUNING_ENABLE /* enable math dynamic pruning */

//#define HIGHLIGHT_MATH_ALIGNMENT /* highlight info (qmask and dmask) */

//#define SKIP_SEARCH

// #define MATH_PRUNING_DISABLE_JUMP /* disable jumping/skipping */
// #define DEBUG_MERGE_SKIPPING /* show all the jumping/skipping points */

// #define DEBUG_MATH_SCORE_INSPECT /* particular expID scoring inspect */
// #define DEBUG_PRINT_QRY_STRUCT

#define MATH_PRUNING_EARLY_DROP_ENABLE
#define MATH_PRUNING_SECTR_DROP_ENABLE

#define WILDCARD_PATH_QUERY_EXPAND_ENABLE

// #define MATH_SKIP_SET_FAST_SELECTION
// #define DEBUG_MATH_SKIP_SET_SELECTION

// #define PRINT_RECUR_MERGING_ITEMS

// #define MERGE_TIME_LOG "merge.runtime.dat"

//#define DEBUG_MATH_PRUNING

//#define DEBUG_MATH_MERGE

//#define HIGHLIGHT_MATH_W_DEGREE

#define MATH_OCCUR_ONLY_ONE

//#define MATH_PRUNING_MIN_THRESHOLD_FACTOR 0.30f /* aggressive */
//#define MATH_PRUNING_MIN_THRESHOLD_FACTOR .15f  /* conservative */
#define MATH_PRUNING_MIN_THRESHOLD_FACTOR .00f  /* rank-safe */

#ifdef DEBUG_MATH_PRUNING
	#define DEBUG_MERGE_LIMIT_ITERS UINT64_MAX
	#define RANK_SET_DEFAULT_VOL 200
#else
	// #define RANK_SET_DEFAULT_VOL 100
	// #define RANK_SET_DEFAULT_VOL 200
	#define RANK_SET_DEFAULT_VOL 1000
#endif

#define DEFAULT_RES_PER_PAGE 10

//#define DEBUG_PROXIMITY
#define ENABLE_PROXIMITY_SCORE

#define MAX_MATH_OCCURS 16

#define MAX_TOTAL_OCCURS 32

//#define DEBUG_HILIGHT_SEG_OFFSET
//#define DEBUG_HILIGHT_SEG

/* prefix search */
#define MAX_NODE_IDS 4096
#define MAX_CELL_CNT 512

//#define MATH_PREFIX_QRY_DEBUG_PRINT

#define MAX_MATH_EXPR_SIM_SCALE 1000

#ifdef MATH_SLOW_SEARCH
	#define MATH_COMPUTE_R_CNT /* compute internode node mapping/count */
	#define CNT_VISIBLE_NODES_ONLY /* count operator only if it is visible */
	#define MAX_MTREE 1
#else
	#define MAX_MTREE 1
#endif

/* plus 3 to be safe, since some places mask[] index is harded coded. */
#define HIGHLIGHT_MTREE_ALLOC (MAX_MTREE + 3)

#define MAX_LEAVES  64

// #define SKIP_SEARCH /* for debug */
// #define PRINT_RECUR_MERGING_ITEMS /* for debug */
