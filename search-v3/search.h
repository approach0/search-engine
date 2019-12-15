#pragma once

/* dependency headers */
#include "indices-v3/config.h" /* for INDICES_TXT_LEXER */
#include "indices-v3/indices.h"
#include "merger/config.h" /* for MAX_MERGE_SET_SZ */
#include "query.h"
#include "rank.h"

/* stats sync array for parallel search */
typedef struct {
	uint32_t docN;
	uint32_t avgDocLen;
	uint32_t pathN;
	uint32_t doc_freq[MAX_MERGE_SET_SZ];
} indices_run_sync_t;

#define MAX_SYNC_ARR_LEN (sizeof(indices_run_sync_t) / sizeof(uint32_t))

/* indices query function */
ranked_results_t
indices_run_query(struct indices*, struct query*, indices_run_sync_t *,
                  int, FILE*);
