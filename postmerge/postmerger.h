#pragma once
#include <stdint.h>
#include <stddef.h> /* for size_t */
#include "config.h"

typedef uint64_t (*postmerger_callbk_cur)(void *);
typedef int      (*postmerger_callbk_next)(void *);
typedef int      (*postmerger_callbk_jump)(void *, uint64_t);
typedef size_t   (*postmerger_callbk_read)(void *po, void *dest, size_t);
typedef int      (*postmerger_callbk_init)(void *);
typedef void     (*postmerger_callbk_uninit)(void *);

struct postmerger_postlist {
	void                    *po;
	postmerger_callbk_cur    cur;
	postmerger_callbk_next   next;
	postmerger_callbk_jump   jump;
	postmerger_callbk_read   read;
	postmerger_callbk_init   init;
	postmerger_callbk_uninit uninit;
};

struct postmerger {
	struct postmerger_postlist po[MAX_MERGE_POSTINGS];
	int n_po;
};

void postmerger_init(struct postmerger*);

#define POSTMERGER_POSTLIST_CALL(_pm, _fun, _i, ...) \
	(*((_pm)->po[_i]._fun)) ((_pm)->po[_i].po, ##__VA_ARGS__)

/* iterator-related */
typedef struct postmerger_iterator {
	uint64_t min;
	int size, map[MAX_MERGE_POSTINGS];
	struct postmerger *pm;
} *postmerger_iter_t;

postmerger_iter_t
postmerger_iterator(struct postmerger*);

int postmerger_empty(struct postmerger*);

void
postmerger_iter_free(postmerger_iter_t);

int
postmerger_iter_next(postmerger_iter_t);

#define postmerger_iter_call(_pm, _iter, _fun, _i, ...) \
	POSTMERGER_POSTLIST_CALL(_pm, _fun, (_iter)->map[_i], ##__VA_ARGS__)

void postmerger_iter_remove(postmerger_iter_t, int);
