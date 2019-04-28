#pragma once
#include <stdint.h>
#include <stddef.h> /* for size_t */
#include "config.h"

typedef uint64_t (*postmerger_callbk_cur)(void *);
typedef int      (*postmerger_callbk_next)(void *);
typedef int      (*postmerger_callbk_jump)(void *, uint64_t);
typedef size_t   (*postmerger_callbk_read)(void *, void *, size_t);
typedef void*    (*postmerger_callbk_get)(void *); /* get iterator */
typedef void     (*postmerger_callbk_del)(void *); /* del iterator */

typedef struct postmerger_postlist {
	void                    *po;
	postmerger_callbk_cur    cur;
	postmerger_callbk_next   next;
	postmerger_callbk_jump   jump;
	postmerger_callbk_read   read;
	postmerger_callbk_get    get;
	postmerger_callbk_del    del;
} pm_po_t;

typedef struct postmerger {
	int       n_po;
	pm_po_t  *po[MAX_MERGE_POSTINGS];
	void*     iter[MAX_MERGE_POSTINGS];
	int       map[MAX_MERGE_POSTINGS]; /* iter[i] --> po[pid] */
	uint64_t  min;
} *postmerger_iter_t;

#define POSTMERGER_POSTLIST_CALL(_pm, _fun, _pid, ...) \
	(*((_pm)->po[_pid]->_fun)) ((_pm)->iter[_pid], ##__VA_ARGS__)

#define postmerger_iter_call(_pm, _fun, _i, ...) \
	POSTMERGER_POSTLIST_CALL(_pm, _fun, (_pm)->map[_i], ##__VA_ARGS__)

postmerger_iter_t
postmerger_iterator(struct postmerger_postlist*, int);

int postmerger_empty(struct postmerger*);

int postmerger_iter_next(postmerger_iter_t);

void postmerger_iter_remove(postmerger_iter_t, int);

void postmerger_iter_free(postmerger_iter_t);
