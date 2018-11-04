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

// #define PRINT_RECUR_MERGING_ITEMS

// #define DEBUG_MATH_EXPR_SEARCH
// #define DEBUG_MATH_SCORE_INSPECT

//#define DEBUG_STATS_HOT_HIT

#define MATH_PRUNING_ENABLE

// #define DEBUG_MATH_PRUNING
// #define DEBUG_PRINT_QRY_STRUCT

#define MATH_PRUNING_MIN_THRESHOLD_FACTOR .125f /* aggressive */
//#define MATH_PRUNING_MIN_THRESHOLD_FACTOR .08f  /* conservative */
//#define MATH_PRUNING_MIN_THRESHOLD_FACTOR .00f  /* rank-safe */

#ifdef DEBUG_MATH_PRUNING
	#define DEBUG_MATH_PRUNING_LIMIT_ITERS 90
	#define RANK_SET_DEFAULT_VOL 3
#else
	// #define RANK_SET_DEFAULT_VOL 50
	   #define RANK_SET_DEFAULT_VOL 200
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

#define MAX_MATH_EXP_SEARCH_ITEMS 9301000
// #define MAX_MATH_EXP_SEARCH_ITEMS 301000
#define MAX_MATH_EXP_SEARCH_DIRS  521

#define MATCH_DIM_WEIGHT 10000

/* prefix search */
#define MAX_NODE_IDS 4096
#define MAX_CELL_CNT 512

//#define MATH_PREFIX_QRY_DEBUG_PRINT

#define MAX_MATH_EXPR_SIM_SCALE 1000

/* switch between SLOW/FAST search */
// #define MATH_SLOW_SEARCH

#ifdef MATH_SLOW_SEARCH
	#define MATH_COMPUTE_R_CNT /* compute internode node mapping/count */
	#define MAX_MTREE 3
	#undef MATH_PRUNING_ENABLE
#else
	#define MAX_MTREE 1
#endif

#define MAX_LEAVES  64

#ifdef MATH_SLOW_SEARCH
	#define HIGHLIGHT_MATH_ALIGNMENT
	#define CNT_VISIBLE_NODES_ONLY
#endif

//#define PRINT_MATH_POST_TYPE
