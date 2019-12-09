#pragma once

#define MAX_GENERPATH_INDEX_LEVEL 2

#include "tex-parser/config.h" /* for MAX_SUBPATH_ID */
#define MAX_MATH_PATHS MAX_SUBPATH_ID /* max number of leaf-root paths */

#define GENER_PATH_NAME  "gener"
#define PREFIX_PATH_NAME "prefix"

#define MSTATS_FNAME "mstats" // math index stats

#define MINVLIST_FNAME "minvlist" // math inverted list
#define SYMBINFO_FNAME "symbinfo" // symbol information
#define PATHFREQ_FNAME "pathfreq" // token path frequency

#include "skippy/ondisk-skippy.h"
#define MATH_INDEX_BLK_LEN ON_DISK_SKIPPY_SKIPPY_SPANS

// #define DEBUG_SUBPATH_SET

// #define MATH_INDEX_SECTTR_PRINT
