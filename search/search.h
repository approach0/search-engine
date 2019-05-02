#pragma once
#include "indices/indices.h"

#include "config.h"
#include "rank.h"
#include "snippet.h"
#include "query.h"

ranked_results_t
indices_run_query(struct indices*, struct query*);

#ifdef DEBUG_MATH_MERGE
extern int g_debug_print;
int debug_search_slowdown();
#endif
