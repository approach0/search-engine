#include <stddef.h> /* for size_t */
#include <stdint.h>
#include "config.h"

typedef uint64_t (*merger_callbk_cur)(void *);
typedef int      (*merger_callbk_next)(void *);
typedef int      (*merger_callbk_skip)(void *, uint64_t);
typedef size_t   (*merger_callbk_read)(void *, void *, size_t);

typedef struct merge_set {
	unsigned int        n; /* number of members */
	void               *iter[MAX_MERGE_SET_SZ]; /* inverted list iterator */
	float                upp[MAX_MERGE_SET_SZ]; /* score upperbound */
	float             pd_upp[MAX_MERGE_SET_SZ]; /* podium upperbound */
	float             sortby[MAX_MERGE_SET_SZ]; /* sort key */
	merger_callbk_cur    cur[MAX_MERGE_SET_SZ]; /* current key callback */
	merger_callbk_next  next[MAX_MERGE_SET_SZ]; /* next key callback */
	merger_callbk_skip  skip[MAX_MERGE_SET_SZ]; /* `skip to' callback */
	merger_callbk_read  read[MAX_MERGE_SET_SZ]; /* read item callback */
} merge_set_t;

int merger_set_empty(struct merge_set *);

uint64_t empty_invlist_cur(void*);
int      empty_invlist_next(void*);
int      empty_invlist_skip(void*, uint64_t);
size_t   empty_invlist_read(void*, void*, size_t);
