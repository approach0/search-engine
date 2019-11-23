#pragma once
#include <stdint.h>
#include "minheap/minheap.h"
#include "config.h"

struct rank_hit {
	uint32_t     docID;
	float        score;
	uint32_t     n_occurs;
	uint32_t    *occur; /* occur positions */
};

typedef struct priority_Q {
	struct heap heap;
	uint32_t    n_elements;
} ranked_results_t;

void  priority_Q_init(struct priority_Q*, uint32_t);
bool  priority_Q_full(struct priority_Q*);
float priority_Q_min_score(struct priority_Q*);
bool  priority_Q_add_or_replace(struct priority_Q*, struct rank_hit*);
void  priority_Q_sort(struct priority_Q*);
void  priority_Q_print(struct priority_Q*);
void  priority_Q_free(struct priority_Q*);
struct rank_hit *priority_Q_top(struct priority_Q*);
uint32_t         priority_Q_len(struct priority_Q*);

#define free_ranked_results(_Q) \
	priority_Q_free(_Q)

/* ranking window */
struct rank_wind {
	ranked_results_t *results;
	int              from, to;
};

struct rank_result {
	struct rank_hit *hit;
	int from, cnt, to, cur;
};

struct rank_wind
rank_wind_calc(ranked_results_t*, int, int, int*);

typedef void (*rank_wind_callbk)(struct rank_result*, void*);

int rank_wind_foreach(struct rank_wind*, rank_wind_callbk, void*);
