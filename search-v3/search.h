#pragma once
/* dependency headers */
#include "indices-v3/indices.h"
#include "query.h"
#include "rank.h"

ranked_results_t indices_run_query(struct indices*, struct query*);
