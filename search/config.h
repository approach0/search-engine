/* for MAX_TXT_SEG_BYTES & MAX_TXT_SEG_LEN */
#include "txt-seg/config.h"
#define MAX_TERM_BYTES MAX_TXT_SEG_BYTES

/* consider both math & term */
#define MAX_QUERY_BYTES     (MAX_TXT_SEG_BYTES * 32)
#define MAX_QUERY_WSTR_LEN  (MAX_TXT_SEG_LEN * 32)

#define MAX_POSTINGS_PER_MATH (MAX_MERGE_POSTINGS >> 3)

#define SNIPPET_PADDING    320
#define MAX_TEX_STRLEN    (MAX_TXT_SEG_BYTES * 32)
#define MAX_SNIPPET_SZ     8192

//#define DEBUG_SNIPPET

//#define DEBUG_POST_MERGE

/* max mark score, type of mnc_score_t */
#define MNC_MARK_BASE_SCORE 90
#define MNC_MARK_FULL_SCORE 100

/*
#define MNC_DEBUG
#define MNC_SMALL_BITMAP
*/

#define MATH_SYMBOLIC_SCORING_ENABLE /* enable symbolic scoring */
#define MATH_PRUNING_ENABLE /* enable math dynamic pruning */
// #define MATH_PRUNING_DISABLE_JUMP /* disable jumping/skipping */
// #define DEBUG_MERGE_SKIPPING /* show all the jumping/skipping points */
// #define DEBUG_MATH_SCORE_INSPECT /* particular expID scoring inspect */

// #define PRINT_RECUR_MERGING_ITEMS

// #define DEBUG_MATH_EXPR_SEARCH

//#define DEBUG_STATS_HOT_HIT


// #define QUIET_SEARCH
// #define MERGE_TIME_LOG "merge.runtime.dat"

// #define DEBUG_MERGE_LIMIT_ITERS 900

// #define DEBUG_MATH_PRUNING
// #define DEBUG_PRINT_QRY_STRUCT

//#define MATH_PRUNING_MIN_THRESHOLD_FACTOR 0.125f /* aggressive */
//#define MATH_PRUNING_MIN_THRESHOLD_FACTOR .08f  /* conservative */
#define MATH_PRUNING_MIN_THRESHOLD_FACTOR .00f  /* rank-safe */

#ifdef DEBUG_MATH_PRUNING
	#define DEBUG_MERGE_LIMIT_ITERS 2
	#define RANK_SET_DEFAULT_VOL 200
#else
	// #define RANK_SET_DEFAULT_VOL 50
	#define RANK_SET_DEFAULT_VOL 1000
	// #define RANK_SET_DEFAULT_VOL 1000
#endif

#define DEFAULT_RES_PER_PAGE 10

//#define DEBUG_PROXIMITY
//#define ENABLE_PROXIMITY_SCORE

#define MATH_PREFIX_SEARCH_ONLY /* determine lr_search or prefix_search */

//#define ENABLE_PARTIAL_MATCH_PENALTY

#define MAX_HIGHLIGHT_OCCURS 8
//#define DEBUG_HILIGHT_SEG_OFFSET
//#define DEBUG_HILIGHT_SEG

//#define DEBUG_MATH_SCORE_POSTING
#define VERBOSE_SEARCH

//#define DEBUG_MATH_SEARCH

//#define DEBUG_PRINT_TARGET_DOC_MATH_SCORE

#define MAX_MATH_EXP_SEARCH_ITEMS 9301000 /* dead */
#define MAX_MATH_EXP_SEARCH_DIRS  521     /* dead */

#define MATCH_DIM_WEIGHT 10000

/* prefix search */
#define MAX_NODE_IDS 4096
#define MAX_CELL_CNT 512

//#define MATH_PREFIX_QRY_DEBUG_PRINT

#define MAX_MATH_EXPR_SIM_SCALE 1000

#ifdef MATH_SLOW_SEARCH
	#define MATH_COMPUTE_R_CNT /* compute internode node mapping/count */
	#define HIGHLIGHT_MATH_ALIGNMENT /* highlight info (qmask and dmask) */
	#define CNT_VISIBLE_NODES_ONLY /* count operator only if it is visible */
	#define MAX_MTREE 1
#else
	#define MAX_MTREE 1
#endif

#define HIGHLIGHT_MTREE_ALLOC (MAX_MTREE + 3)

#define MAX_LEAVES  64

//#define PRINT_MATH_POST_TYPE
