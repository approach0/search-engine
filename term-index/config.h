#pragma once

#define MAX_TERM_ITEM_POSITIONS 8
#define MAX_TERM_MERGE_POSTINGS 4096

/* macro defined in "indri/src/RepositoryMaintenanceThread.cpp" */
#define MAXIMUM_INDEX_COUNT 5

#define TERM_INDEX_BLK_LEN 32

#define PRINT_CACHING_TEXT_TERMS
#define MINIMAL_CACHE_DOC_FREQ (TERM_INDEX_BLK_LEN * TERM_INDEX_BLK_LEN)

#define TERM_INDEX_LONG_WORD_PLACEHOLDER "_word_too_long_"
#define TERM_INDEX_MATH_EXPR_PLACEHOLDER "_math_formula_"
