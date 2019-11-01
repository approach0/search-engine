#include <stddef.h> /* for size_t */
#include <stdint.h>
#include "config.h"

typedef uint64_t (*merger_callbk_cur)(void *);
typedef int      (*merger_callbk_next)(void *);
typedef int      (*merger_callbk_skip)(void *, uint64_t);
typedef size_t   (*merger_callbk_read)(void *, void *, size_t);

typedef float    (*upp_relax_callbk)(void *, float);

typedef struct merge_set {
	unsigned int        n; /* number of members */
	void               *iter[MAX_MERGE_SET_SZ];
	float                upp[MAX_MERGE_SET_SZ];
	merger_callbk_cur    cur[MAX_MERGE_SET_SZ];
	merger_callbk_next  next[MAX_MERGE_SET_SZ];
	merger_callbk_skip  skip[MAX_MERGE_SET_SZ];
	merger_callbk_read  read[MAX_MERGE_SET_SZ];
} merge_set_t;
