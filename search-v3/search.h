#pragma once
/* dependency headers */
#include "indices-v3/config.h" /* for INDICES_TXT_LEXER */
#include "indices-v3/indices.h"
#include "query.h"
#include "rank.h"

typedef struct {
	uint32_t docN;
	uint32_t avgDocLen;
	uint32_t pathN;
	uint    *doc_freq;
} indices_run_sync_t;

ranked_results_t
indices_run_query(struct indices*, struct query*, indices_run_sync_t *, int);
