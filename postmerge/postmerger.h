#pragma once
#include <stdint.h>
#include <stddef.h> /* for size_t */
#include "config.h"

typedef uint64_t (*postmerger_callbk_cur)(void *);
typedef int      (*postmerger_callbk_next)(void *);
typedef int      (*postmerger_callbk_jump)(void *, uint64_t);
typedef size_t   (*postmerger_callbk_read)(void *, void *, size_t);
typedef void*    (*postmerger_callbk_get_iter)(void *); /* get iterator */
typedef void     (*postmerger_callbk_del_iter)(void *); /* del iterator */

typedef struct postmerger_postlist {
	void                       *po;
	postmerger_callbk_cur       cur;
	postmerger_callbk_next      next;
	postmerger_callbk_jump      jump;
	postmerger_callbk_read      read;
	postmerger_callbk_get_iter  get_iter;
	postmerger_callbk_del_iter  del_iter;
} pm_po_t;

struct postmerger_postlists {
	pm_po_t   po[MAX_MERGE_POSTINGS];
	int       n;
};

typedef struct postmerger {
	int       size; /* dynamical number of active iterators */
	pm_po_t   po[MAX_MERGE_POSTINGS];
	int       map[MAX_MERGE_POSTINGS]; /* iter[i] --> po[pid] */
	void*     iter[MAX_MERGE_POSTINGS];
	uint64_t  min;
} *postmerger_iter_t;

#define POSTMERGER_POSTLIST_FUN(_pm, _fun, _pid, ...) \
	(*((_pm)->po[_pid]._fun)) (__VA_ARGS__)

#define POSTMERGER_POSTLIST_CALL(_pm, _fun, _pid, ...) \
	(*((_pm)->po[_pid]._fun)) ((_pm)->iter[_pid], ##__VA_ARGS__)

#define postmerger_iter_call(_pm, _fun, _i, ...) \
	POSTMERGER_POSTLIST_CALL(_pm, _fun, (_pm)->map[_i], ##__VA_ARGS__)

postmerger_iter_t postmerger_iterator(struct postmerger_postlists*);

int postmerger_empty(struct postmerger_postlists*);

int postmerger_iter_next(postmerger_iter_t);

void postmerger_iter_remove(postmerger_iter_t, int);

void postmerger_iter_free(postmerger_iter_t);
