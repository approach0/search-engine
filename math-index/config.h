#pragma once

#define GENER_PATH_NAME "gener"
#define TOKEN_PATH_NAME "token"
#define PREFIX_PATH_NAME "prefix"

#define MATH_POSTING_FNAME "posting.bin"
#define PATH_INFO_FNAME    "pathinfo.bin"

#define DISK_BLCK_SIZE 4096

/* DISK_RD_BLOCKS is 3 here because math_posting_item structure
 * has 3 members, this makes fread() reads integral blocks.
 * (before compression on disk is implemented) */
#define DISK_RD_BLOCKS 3

//#define DEBUG_SUBPATH_SET

//#define DEBUG_DIR_MERGE

//#define DEBUG_MATH_POSTING

//#define DEBUG_MATH_INDEX

#include "postmerge/config.h" /* for MAX_MERGE_POSTINGS */

#define MAX_MERGE_DIRS MAX_MERGE_POSTINGS

#define MAX_WILDCARD_LEVEL 2
