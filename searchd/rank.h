#include <stdint.h>
#include "minheap.h"

typedef uint32_t frml_id_t;

struct rank_item {
	doc_id_t    docID;
	char      **terms;
	uint32_t    hit_terms;
	frml_id_t **frmlIDs;
	float       score;
};

struct rank_set {
	struct heap heap;
	uint32_t    n_terms, n_frmls;
	uint32_t    n_items;
};

#define RANK_SET_DEFAULT_SZ  45
#define DEFAULT_RES_PER_PAGE 10

void rank_init(struct rank_set*, uint32_t, uint32_t, uint32_t);
void rank_uninit(struct rank_set*);

void
rank_cram(struct rank_set*, doc_id_t, float, uint32_t,
          char **, frml_id_t **);

void rank_sort(struct rank_set*);
void rank_print(struct rank_set*);

/* ranking window */
struct rank_wind {
	struct rank_set *rs;
	uint32_t   from, to;
};

struct rank_wind
rank_window_calc(struct rank_set*, uint32_t, uint32_t, uint32_t*);

typedef void (*rank_wind_callbk)(struct rank_set *, struct rank_item*,
                                 uint32_t, void*);

uint32_t
rank_window_foreach(struct rank_wind*, rank_wind_callbk, void*);
