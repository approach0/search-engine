#pragma once

#define MAX_TERM_ITEM_POSITIONS 8
#define MAX_TERM_MERGE_POSTINGS 4096

/* macro defined in "indri/src/RepositoryMaintenanceThread.cpp" */
#define MAXIMUM_INDEX_COUNT 5

// #define IGNORE_TERM_INDEX

#define TERM_INDEX_BLK_LEN 32

#define PRINT_CACHING_TEXT_TERMS
#define MINIMAL_CACHE_DOC_FREQ  0 // avgDocLen
