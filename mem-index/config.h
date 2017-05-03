//#define DEBUG_MEM_POSTING
//#define DEBUG_MEM_POSTING_SMALL_BUF_SZ 63
//#define DEBUG_MEM_POSTING_SHOW_COMPRESS_RATE

#include "../term-index/config.h"

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
/* ROUND_UP(12, 5) should be 5 * (12/5 + 1) = 15 */
/* ROUND_UP(10, 5) should be 10 * (10/5 + 0) = 10 */


#define MIN_MEM_POSTING_BUF_SZ \
	(sizeof(doc_id_t) + sizeof(uint32_t) + \
	MAX_TERM_INDEX_ITEM_POSITIONS * sizeof(position_t))

#define MEM_POSTING_BUF_SZ ROUND_UP(MIN_MEM_POSTING_BUF_SZ, 4096)
