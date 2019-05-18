#pragma once
#include <stdint.h>
#include "minheap/minheap.h"
#include "config.h"

typedef struct hit_occur {
	position_t  pos;
#ifdef HIGHLIGHT_MATH_ALIGNMENT
	uint64_t    qmask[HIGHLIGHT_MTREE_ALLOC];
	uint64_t    dmask[HIGHLIGHT_MTREE_ALLOC];
#endif
} hit_occur_t;

struct rank_hit {
	doc_id_t     docID;
	float        math_score, text_score, score;
	uint32_t     n_occurs;
	hit_occur_t *occurs; /* occur positions */
};

typedef struct priority_Q {
	struct heap heap;
	uint32_t    n_elements;
} ranked_results_t /* a conceptually more descriptive name */;

void  priority_Q_init(struct priority_Q*, uint32_t);
bool  priority_Q_full(struct priority_Q*);
float priority_Q_min_score(struct priority_Q*);
bool  priority_Q_add_or_replace(struct priority_Q*, struct rank_hit*);
void  priority_Q_sort(struct priority_Q*);
void  priority_Q_print(struct priority_Q*);
void  priority_Q_free(struct priority_Q*);
struct rank_hit *priority_Q_top(struct priority_Q*);
uint32_t         priority_Q_len(struct priority_Q*);

/* a conceptually more descriptive name */
#define free_ranked_results(_Q) \
	priority_Q_free(_Q)

/* ranking window */
struct rank_window {
	ranked_results_t *results;
	uint32_t          from, to;
};

struct rank_window
rank_window_calc(ranked_results_t*, int, uint32_t, uint32_t*);

typedef void (*rank_window_it_callbk)(struct rank_hit*, uint32_t, void*);

uint32_t
rank_window_foreach(struct rank_window*, rank_window_it_callbk, void*);

typedef void (*rank_window_it_callbk2)(struct rank_hit*, uint32_t, uint32_t, void*);

uint32_t
rank_window_foreach2(struct rank_window*, rank_window_it_callbk2, void*);
