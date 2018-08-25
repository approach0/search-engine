#include <stdint.h>
#include "config.h"

typedef uint64_t (*postmerger_callbk_cur)(void *);
typedef int      (*postmerger_callbk_next)(void *);

struct postmerger_postlist {
	void                   *po;
	postmerger_callbk_cur   cur;
	postmerger_callbk_next  next;
};

struct postmerger {
	struct postmerger_postlist po[MAX_MERGE_POSTINGS];
	int n_po;
};

void postmerger_init(struct postmerger*);

#define POSTMERGER_POSTLIST_CALL(_pm, _fun, _i, ...) \
	(*((_pm)->po[_i]._fun)) ((_pm)->po[_i].po, ##__VA_ARGS__)

/* iterator-related */
struct postmerger_iterator {
	uint64_t min;
	int size, map[MAX_MERGE_POSTINGS];
};

int postmerger_empty(struct postmerger*);

struct postmerger_iterator
postmerger_iterator(struct postmerger*);

int
postmerger_iter_next(struct postmerger*, struct postmerger_iterator*);

#define postmerger_iter_call(_pm, _iter, _fun, _i, ...) \
	POSTMERGER_POSTLIST_CALL(_pm, _fun, (_iter)->map[_i], ##__VA_ARGS__)

void postmerger_iter_remove(struct postmerger_iterator*, int);
