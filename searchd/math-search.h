#include "math-index/math-index.h"
#include "math-index/subpath-set.h"

#include "postmerge.h"
#include "mnc-score.h"

/* perform math expression search and upon math posting list merge,
 * call the callback function specified in the argument. */
int math_search_posting_merge(math_index_t, char*, enum dir_merge_type,
                              post_merge_callbk, void*);
