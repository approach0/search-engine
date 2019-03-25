#pragma once
#include "indices/indices.h"

#include "config.h"
#include "rank.h"
#include "snippet.h"
#include "query.h"

ranked_results_t
indices_run_query(struct indices*, struct query*);
